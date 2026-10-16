#include "zoom.hpp"

#include "geometry.hpp"

namespace topscreen::core {

namespace {
double clamp_range(double v, double lo, double hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}
} // namespace

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
	last_focus_ms_ = now_ms;
	last_update_ms_ = now_ms;
	started_ = true;
}

void ZoomModel::focus_on(double x, double y, double now_ms)
{
	if (!started_)
		reset(now_ms);
	fx_.set_target(x);
	fy_.set_target(y);
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

	/* Pan: reduz a amplitude do foco conforme pan_amount. */
	const double pan = clamp_range(cfg_.pan_amount, 0.0, 1.0);
	if (pan > 0.0) {
		const double cx = 0.5 + (fx_.target() - 0.5) * pan;
		const double cy = 0.5 + (fy_.target() - 0.5) * pan;
		fx_.set_target(cx);
		fy_.set_target(cy);
	}

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
