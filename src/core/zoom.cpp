#include "zoom.hpp"

#include <algorithm>

#include "geometry.hpp"
#include "math.hpp"

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
	last_update_ms_ = now_ms;
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

void ZoomModel::spot_start(double cx, double cy, int direction, double factor, double max_zoom,
			   double in_ms, double hold_ms, double out_ms, double now_ms)
{
	spot_active_ = true;
	spot_cx_ = clamp_range(cx, 0.0, 1.0);
	spot_cy_ = clamp_range(cy, 0.0, 1.0);
	spot_dir_ = direction;

	/* The gesture starts from what is on screen right now (which may already be
	 * zoomed) and returns to it. */
	spot_base_ = std::max(1.0, zoom_.value());
	spot_base_fx_ = fx_.value();
	spot_base_fy_ = fy_.value();

	const double f = factor > 1.0 ? factor : 1.0;
	const double upper = max_zoom > 1.0 ? max_zoom : 8.0;
	if (direction == 1 || direction == 3)
		spot_target_ = clamp_range(spot_base_ / f, 1.0, 8.0);
	else
		spot_target_ = clamp_range(spot_base_ * f, 1.0, upper);

	/* Punch: no hold — it goes in and immediately comes back. */
	const bool punch = direction >= 2;
	spot_in_ms_ = in_ms > 1.0 ? in_ms : 1.0;
	spot_hold_ms_ = punch ? 0.0 : (hold_ms >= 0.0 ? hold_ms : 0.0);
	spot_out_ms_ = out_ms > 1.0 ? out_ms : 1.0;
	spot_start_ms_ = now_ms;
}

void ZoomModel::spot_stop()
{
	spot_active_ = false;
}

double ZoomModel::spot_zoom_at(double now_ms) const
{
	const double elapsed = now_ms - spot_start_ms_;
	if (elapsed <= spot_in_ms_) {
		/* Quick ease-out into the target (punch is snappier). */
		const double u = elapsed / spot_in_ms_;
		const double e = spot_dir_ >= 2 ? u * u : 1.0 - (1.0 - u) * (1.0 - u) * (1.0 - u);
		return spot_base_ + (spot_target_ - spot_base_) * e;
	}
	if (elapsed <= spot_in_ms_ + spot_hold_ms_)
		return spot_target_;

	/* Slow, smooth return to the state captured at the selection. */
	const double u = (elapsed - spot_in_ms_ - spot_hold_ms_) / spot_out_ms_;
	if (u >= 1.0)
		return spot_base_;
	const double e = u * u * (3.0 - 2.0 * u); /* smoothstep */
	return spot_target_ + (spot_base_ - spot_target_) * e;
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

	if (spot_active_ && now_ms - spot_start_ms_ > spot_in_ms_ + spot_hold_ms_ + spot_out_ms_)
		spot_active_ = false;

	double dt = (now_ms - last_update_ms_) / 1000.0;
	last_update_ms_ = now_ms;
	if (dt < 0.0)
		dt = 0.0;

	const bool auto_mode = cfg_.mode == static_cast<int>(ZoomMode::Auto);
	const bool active = auto_mode && (now_ms - last_focus_ms_) < cfg_.idle_timeout_ms;

	double target_zoom = manual_;
	if (active)
		target_zoom = manual_ * (cfg_.amount > 1.0 ? cfg_.amount : 1.0);
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

	const FocusLimits lx = focus_limits(focus_point_x_, target_zoom);
	const FocusLimits ly = focus_limits(focus_point_y_, target_zoom);
	fx_.set_target(clamp_range(focus_target(focus_point_x_), lx.lo, lx.hi));
	fy_.set_target(clamp_range(focus_target(focus_point_y_), ly.lo, ly.hi));

	zoom_.update(dt);
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
	return out;
}

} // namespace topscreen::core
