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
			x_.store(nx);
			y_.store(ny);
			known_.store(true);
			blog(LOG_INFO, "[TopScreen] monitor origin: (%.0f,%.0f) [manual]", nx, ny);
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
	if (obs_source_t *parent = obs_filter_get_parent(filter)) {
		obs_data_t *s = obs_source_get_settings(parent);
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
			obs_data_release(s);
		}
	}

	double ox = 0.0;
	double oy = 0.0;
	const bool known = ts_resolve_monitor_origin(device.c_str(), hint, trust_hint, target_w,
						     target_h, &ox, &oy);
	x_.store(known ? ox : 0.0);
	y_.store(known ? oy : 0.0);
	known_.store(known);
}

} // namespace topscreen
