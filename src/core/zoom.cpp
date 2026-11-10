#include "zoom.hpp"

#include <cmath>

#include <algorithm>

#include "geometry.hpp"
#include "math.hpp"
#include "easing.hpp"

namespace topscreen::core {

double ZoomModel::stiffness() const
{
	const double d = cfg_.duration_ms / 1000.0;
	return 100.0 / (d > 0.05 ? d : 0.05);
}

void ZoomModel::configure(const ZoomConfig &cfg)
{
	cfg_ = cfg;
	const double k = stiffness();
	zoom_.set_stiffness(k);
	fx_.set_stiffness(k);
	fy_.set_stiffness(k);
}

void ZoomModel::reset(double now_ms)
{
	manual_ = 1.0;
	zoom_.reset(1.0);
	fx_.reset(0.5);
	fy_.reset(0.5);
	focus_point_x_ = 0.5;
	focus_point_y_ = 0.5;
	last_focus_ms_ = now_ms;
	last_activity_ms_ = -1.0e9;
	region_valid_ = false;
	last_update_ms_ = now_ms;
	velocity_ = 0.0;
	started_ = true;
}

void ZoomModel::focus_on(double x, double y, double now_ms)
{
	if (!started_)
		reset(now_ms);

	/* Saves the focused point; the focus target is recalculated in update
	 * (it depends on the zoom, which animates). */
	focus_point_x_ = clamp_range(x, 0.0, 1.0);
	focus_point_y_ = clamp_range(y, 0.0, 1.0);
	last_focus_ms_ = now_ms;
}

void ZoomModel::set_cursor(double x, double y, double now_ms)
{
	focus_on(x, y, now_ms);
}

void ZoomModel::note_activity(double now_ms)
{
	last_activity_ms_ = now_ms;
}

bool ZoomModel::set_region(double x0, double y0, double x1, double y1)
{
	const double nx0 = clamp_range(std::min(x0, x1), 0.0, 1.0);
	const double ny0 = clamp_range(std::min(y0, y1), 0.0, 1.0);
	const double nx1 = clamp_range(std::max(x0, x1), 0.0, 1.0);
	const double ny1 = clamp_range(std::max(y0, y1), 0.0, 1.0);
	const bool valid = (nx1 - nx0) > 0.001 && (ny1 - ny0) > 0.001;
	const bool changed = !region_valid_ || valid != region_valid_ ||
			     nx0 != region_x0_ || ny0 != region_y0_ ||
			     nx1 != region_x1_ || ny1 != region_y1_;
	region_x0_ = nx0;
	region_y0_ = ny0;
	region_x1_ = nx1;
	region_y1_ = ny1;
	region_valid_ = valid;
	return changed;
}

void ZoomModel::clear_region()
{
	region_valid_ = false;
}

void ZoomModel::spot_start_hold(double focus_x, double focus_y, double target, double in_ms,
				  double hold_ms, double out_ms, double now_ms)
{
	spot_active_ = true;
	spot_held_ = true;
	spot_cx_ = clamp_range(focus_x, 0.0, 1.0);
	spot_cy_ = clamp_range(focus_y, 0.0, 1.0);

	/* The gesture starts from what is on screen right now (which may already be
	 * zoomed) and returns to it. */
	spot_base_ = std::max(1.0, zoom_.value());
	spot_base_fx_ = fx_.value();
	spot_base_fy_ = fy_.value();
	spot_target_ = clamp_range(target, 1.0, 8.0);
	spot_release_zoom_ = spot_base_;

	spot_in_ms_ = in_ms > 1.0 ? in_ms : 1.0;
	spot_hold_ms_ = hold_ms >= 0.0 ? hold_ms : 0.0;
	spot_out_ms_ = out_ms > 1.0 ? out_ms : 1.0;
	spot_start_ms_ = now_ms;
	spot_release_ms_ = now_ms;
}

void ZoomModel::spot_release(double now_ms)
{
	if (!spot_active_ || !spot_held_)
		return;
	/* Sample while still held: this is the peak the return starts from. */
	spot_release_zoom_ = spot_zoom_at(now_ms);
	spot_held_ = false;
	spot_release_ms_ = now_ms;
}

void ZoomModel::spot_stop()
{
	spot_active_ = false;
	spot_held_ = false;
}

SpotAim spot_aim_for_hold(bool has_rect, double rx0, double ry0, double rx1, double ry1,
			  double cursor_x, double cursor_y, double base_zoom, double point_zoom,
			  double max_zoom, bool zoom_out)
{
	SpotAim aim;
	const double base = std::max(1.0, base_zoom);
	const double upper = max_zoom > 1.0 ? max_zoom : 8.0;

	double cx = clamp_range(cursor_x, 0.0, 1.0);
	double cy = clamp_range(cursor_y, 0.0, 1.0);
	double target = base;
	if (has_rect) {
		const double rw = std::max(1e-3, rx1 - rx0);
		const double rh = std::max(1e-3, ry1 - ry0);
		const double fit = std::min(1.0 / rw, 1.0 / rh);
		cx = (rx0 + rx1) * 0.5;
		cy = (ry0 + ry1) * 0.5;
		target = zoom_out ? base / std::max(fit, 1.0) : std::min(fit, upper);
	} else if (!zoom_out) {
		target = std::min(std::max(point_zoom, 1.0), upper);
	} else {
		target = base / 2.0;
	}

	if (!zoom_out && target <= base * 1.001)
		return aim; /* no visible change */

	aim.valid = true;
	aim.zoom = std::max(1.0, target);
	if (std::fabs(aim.zoom - base) <= base * 0.001) {
		aim.valid = false; /* clamping ate the whole gesture (e.g. out at 1x) */
		return aim;
	}
	aim.focus_x = center_focus_for(cx, std::max(aim.zoom, 1.0001));
	aim.focus_y = center_focus_for(cy, std::max(aim.zoom, 1.0001));
	return aim;
}

double ZoomModel::spot_zoom_at(double now_ms) const
{	if (spot_held_) {
		/* Ease into the target and stay there while held. */
		const double elapsed = now_ms - spot_start_ms_;
		const double u = std::min(1.0, elapsed / spot_in_ms_);
		const double e = ease_in_out_cubic(u);
		return spot_base_ + (spot_target_ - spot_base_) * e;
	}

	/* Released: optional hold at the target, then a smooth return to the exact
	 * state captured at the selection. */
	if (now_ms < spot_release_ms_ + spot_hold_ms_)
		return spot_release_zoom_;
	const double u = (now_ms - spot_release_ms_ - spot_hold_ms_) / spot_out_ms_;
	if (u >= 1.0)
		return spot_base_;
	const double e = ease_in_out_cubic(u);
	return spot_release_zoom_ + (spot_base_ - spot_release_zoom_) * e;
}

double ZoomModel::spot_focus_at(double now_ms) const
{
	(void)now_ms;
	return spot_cx_;
}

void ZoomModel::manual_step(double factor)
{
	if (factor <= 0.0)
		return;
	manual_ = clamp_range(manual_ * factor, 1.0, 8.0);
}

void ZoomModel::set_manual(double value)
{
	manual_ = clamp_range(value, 1.0, 8.0);
}

void ZoomModel::update(double now_ms)
{
	if (!started_)
		reset(now_ms);

	if (spot_active_ && !spot_held_ &&
	    now_ms - spot_release_ms_ > spot_hold_ms_ + spot_out_ms_)
		spot_active_ = false;

	double dt = (now_ms - last_update_ms_) / 1000.0;
	last_update_ms_ = now_ms;
	if (dt < 0.0)
		dt = 0.0;

	const bool auto_mode = cfg_.mode == static_cast<int>(ZoomMode::Auto);
	const bool smart_mode = cfg_.mode == static_cast<int>(ZoomMode::Smart);
	const bool auto_active =
		auto_mode && (now_ms - last_focus_ms_) < cfg_.idle_timeout_ms;
	const bool smart_active =
		smart_mode && (now_ms - last_activity_ms_) < cfg_.smart_hold_ms;
	const bool active = auto_active || smart_active;
	const bool region_focus = smart_active && region_valid_;

	double target_zoom = manual_;
	if (active)
		target_zoom = manual_ * (cfg_.amount > 1.0 ? cfg_.amount : 1.0);
	if (region_focus) {
		const double rw = std::max(1.0e-4, region_x1_ - region_x0_);
		const double rh = std::max(1.0e-4, region_y1_ - region_y0_);
		const double fit = std::min(1.0 / rw, 1.0 / rh);
		target_zoom = std::min(target_zoom, fit * 0.92);
	}
	target_zoom = clamp_range(target_zoom, 1.0, 8.0);
	zoom_.set_target(target_zoom);

	/*
	 * Focus = blend between "follow the cursor" (pan) and "keep the cursor at
	 * the center" (center_amount). Centering makes the view ANTICIPATE the edges:
	 * the corner appears before the mouse reaches it.
	 */
	const double pan = clamp_range(cfg_.pan_amount, 0.0, 1.0);
	const double camt = clamp_range(cfg_.center_amount, 0.0, 1.0);

	auto focus_target = [&](double s) {
		const double follow = 0.5 + (s - 0.5) * pan;
		const double center = center_focus_for(s, target_zoom);
		return follow + (center - follow) * camt;
	};

	const double focus_x = region_focus ? (region_x0_ + region_x1_) * 0.5
					    : focus_point_x_;
	const double focus_y = region_focus ? (region_y0_ + region_y1_) * 0.5
					    : focus_point_y_;
	const FocusLimits lx = focus_limits(focus_x, target_zoom);
	const FocusLimits ly = focus_limits(focus_y, target_zoom);
	fx_.set_target(clamp_range(focus_target(focus_x), lx.lo, lx.hi));
	fy_.set_target(clamp_range(focus_target(focus_y), ly.lo, ly.hi));

	const double prev_zoom = zoom_.value();
	zoom_.update(dt);
	velocity_ = dt > 1.0e-9 ? (zoom_.value() - prev_zoom) / dt : 0.0;
	fx_.update(dt);
	fy_.update(dt);
}

ZoomRender ZoomModel::render() const
{
	ZoomRender out;

	if (spot_active_) {
		out.zoom = spot_zoom_at(last_update_ms_);
		if (out.zoom < 1.0)
			out.zoom = 1.0;
		/* Focus moves from where it was to the selection and back. */
		const double e = spot_target_ > spot_base_
					 ? (out.zoom - spot_base_) / std::max(1e-6, spot_target_ - spot_base_)
					 : 0.0;
		const double fx = spot_base_fx_ + (clamp_range(spot_cx_, 0.0, 1.0) - spot_base_fx_) * e;
		const double fy = spot_base_fy_ + (clamp_range(spot_cy_, 0.0, 1.0) - spot_base_fy_) * e;
		out.focus_x = clamp_focus(clamp_range(fx, 0.0, 1.0), out.zoom);
		out.focus_y = clamp_focus(clamp_range(fy, 0.0, 1.0), out.zoom);
		return out;
	}

	out.zoom = zoom_.value();
	if (out.zoom < 1.0)
		out.zoom = 1.0;
	out.focus_x = clamp_focus(fx_.value(), out.zoom);
	out.focus_y = clamp_focus(fy_.value(), out.zoom);
	out.velocity = velocity_;
	return out;
}

} // namespace topscreen::core
