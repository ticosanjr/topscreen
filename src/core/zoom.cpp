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
	focus_point_x_ = 0.5;
	focus_point_y_ = 0.5;
	base_fx_ = 0.5;
	base_fy_ = 0.5;
	last_focus_ms_ = now_ms;
	last_update_ms_ = now_ms;
	started_ = true;
}

void ZoomModel::focus_on(double x, double y, double now_ms)
{
	if (!started_)
		reset(now_ms);

	focus_point_x_ = clamp_range(x, 0.0, 1.0);
	focus_point_y_ = clamp_range(y, 0.0, 1.0);

	/* Pan: aproxima o foco do cursor UMA vez (nunca a cada frame). */
	const double pan = clamp_range(cfg_.pan_amount, 0.0, 1.0);
	base_fx_ = 0.5 + (focus_point_x_ - 0.5) * pan;
	base_fy_ = 0.5 + (focus_point_y_ - 0.5) * pan;

	fx_.set_target(base_fx_);
	fy_.set_target(base_fy_);
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
	 * Mantem o ponto focado visivel: se o pan deixaria o cursor fora da tela,
	 * puxa o foco para o limite. Com isso os cantos continuam alcancaveis e o
	 * cursor nao some.
	 */
	const FocusLimits lx = focus_limits(focus_point_x_, target_zoom);
	const FocusLimits ly = focus_limits(focus_point_y_, target_zoom);
	fx_.set_target(clamp_range(base_fx_, lx.lo, lx.hi));
	fy_.set_target(clamp_range(base_fy_, ly.lo, ly.hi));

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
