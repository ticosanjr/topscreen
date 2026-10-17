#include "zoom.hpp"

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
	out.zoom = zoom_.value();
	if (out.zoom < 1.0)
		out.zoom = 1.0;
	out.focus_x = clamp_focus(fx_.value(), out.zoom);
	out.focus_y = clamp_focus(fy_.value(), out.zoom);
	return out;
}

} // namespace topscreen::core
