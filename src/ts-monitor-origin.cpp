#include "ts-monitor-origin.hpp"

#include <cstring>
#include <string>

#include "core/config.hpp"
#include "input/ts-monitor.hpp"

namespace topscreen {

void MonitorOrigin::tick(obs_source_t *filter, const core::Config &cfg, uint32_t target_w,
			 uint32_t target_h)
{
	if (target_w == 0 || target_h == 0)
		return;

	/* TopScreen Capture exposes the selected monitor as a setting. Watch it at
	 * ~0.5 s so switching between identical monitors refreshes immediately
	 * instead of waiting for the 10 s layout poll. */
	if (++check_ticks_ >= 30) {
		check_ticks_ = 0;
		int hint = -1;
		obs_source_t *host = obs_filter_get_parent(filter);
		if (!host && obs_source_get_type(filter) == OBS_SOURCE_TYPE_INPUT)
			host = filter;
		if (host) {
			const char *id = obs_source_get_id(host);
			if (id && std::strcmp(id, "topscreen_capture") == 0) {
				obs_data_t *s = obs_source_get_settings(host);
				if (s) {
					hint = static_cast<int>(
						obs_data_get_int(s, "capture_monitor"));
					obs_data_release(s);
				}
			}
		}
		if (hint != last_capture_monitor_) {
			last_capture_monitor_ = hint;
			age_ticks_ = 600; // force a refresh on this tick
		}
	}

	++age_ticks_;
	refresh(filter, cfg, target_w, target_h);
}

void MonitorOrigin::refresh(obs_source_t *filter, const core::Config &cfg,
			    uint32_t target_w, uint32_t target_h)
{
	if (target_w == 0 || target_h == 0)
		return;

	/* Manual wins: it is the escape hatch when the platform detection cannot
	 * decide (identical monitors stacked below/above each other). */
	if (cfg.monitor_manual) {
		const double nx = static_cast<double>(cfg.monitor_x);
		const double ny = static_cast<double>(cfg.monitor_y);
		if (!known_.load() || nx != x_.load() || ny != y_.load()) {
			/* The manual escape hatch only carries the origin. Find the real
			 * monitor at that origin so a scaled capture still normalizes by
			 * the monitor resolution, not by the capture resolution. */
			double mw = static_cast<double>(target_w);
			double mh = static_cast<double>(target_h);
			core::Monitor monitors[16] = {};
			const int count = ts_query_monitors(monitors, 16);
			for (int i = 0; i < count; ++i) {
				if (monitors[i].x == nx && monitors[i].y == ny) {
					mw = monitors[i].width;
					mh = monitors[i].height;
					break;
				}
			}
			x_.store(nx);
			y_.store(ny);
			width_.store(mw);
			height_.store(mh);
			known_.store(true);
			blog(LOG_INFO, "[TopScreen] monitor origin: (%.0f,%.0f) %.0fx%.0f [manual]",
			     nx, ny, mw, mh);
		}
		age_ticks_ = 0;
		return;
	}

	const bool size_changed = target_w != target_w_ || target_h != target_h_;
	if (!size_changed && age_ticks_ < 600)
		return;

	age_ticks_ = 0;
	target_w_ = target_w;
	target_h_ = target_h;

	/* The capture settings expose the monitor index on Windows (same order as
	 * EnumDisplayMonitors). X11's "screen" is an X screen, not a monitor, so
	 * the hint is not trusted there. */
	int hint = -1;
	bool trust_hint = false;
	std::string device;
	obs_source_t *host = obs_filter_get_parent(filter);
	if (!host && obs_source_get_type(filter) == OBS_SOURCE_TYPE_INPUT)
		host = filter;
	if (host) {
		obs_data_t *s = obs_source_get_settings(host);
		if (s) {
			/* Newer OBS identifies the monitor by device name
			 * (alt_id/id, e.g. "\\.\DISPLAY2"); older builds use an
			 * integer index. Both are optional. */
			for (const char *key : {"alt_id", "device_id", "id"}) {
				const char *v = obs_data_get_string(s, key);
				if (v && *v && std::strncmp(v, "\\\\", 2) == 0) {
					device = v;
					break;
				}
			}
			if (obs_data_has_user_value(s, "monitor") ||
			    obs_data_has_default_value(s, "monitor")) {
				hint = static_cast<int>(obs_data_get_int(s, "monitor"));
				trust_hint = true;
			}
			/* Our own source knows the exact monitor index even when two
			 * monitors have the same resolution. */
			const char *id = obs_source_get_id(host);
			if (id && std::strcmp(id, "topscreen_capture") == 0) {
				hint = static_cast<int>(obs_data_get_int(s, "capture_monitor"));
				trust_hint = true;
			}
			obs_data_release(s);
		}
	}

	core::Monitor monitor;
	const bool known = ts_resolve_monitor(device.c_str(), hint, trust_hint, target_w,
					      target_h, &monitor);
	x_.store(monitor.x);
	y_.store(monitor.y);
	width_.store(monitor.width > 0.0 ? monitor.width : static_cast<double>(target_w));
	height_.store(monitor.height > 0.0 ? monitor.height : static_cast<double>(target_h));
	known_.store(known);
}

} // namespace topscreen
