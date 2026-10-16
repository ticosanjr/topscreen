#include "cursor.hpp"

#include <cmath>

namespace topscreen::core {

namespace {

constexpr double kIdleFadeMs = 300.0;

double clamp01(double v)
{
	return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
}

} // namespace

double CursorModel::stiffness() const
{
	/* smooth 0 -> mola macia, smooth 1 -> mola dura. */
	const double s = clamp01(cfg_.smooth);
	return 20.0 + s * 380.0;
}

void CursorModel::configure(const CursorConfig &cfg)
{
	cfg_ = cfg;
	sx_.set_stiffness(stiffness());
	sy_.set_stiffness(stiffness());
}

void CursorModel::reset(double x, double y, double now_ms)
{
	sx_.reset(x);
	sy_.reset(y);
	start_x_ = x;
	start_y_ = y;
	rotation_ = 0.0;
	last_move_ms_ = now_ms;
	last_update_ms_ = now_ms;
	started_ = true;
}

void CursorModel::set_target(double x, double y, double now_ms)
{
	if (!started_)
		reset(x, y, now_ms);

	if (std::fabs(x - sx_.target()) > 1e-6 || std::fabs(y - sy_.target()) > 1e-6)
		last_move_ms_ = now_ms;

	sx_.set_target(x);
	sy_.set_target(y);
}

void CursorModel::update(double now_ms)
{
	if (!started_)
		return;

	double dt = (now_ms - last_update_ms_) / 1000.0;
	last_update_ms_ = now_ms;
	if (dt < 0.0)
		dt = 0.0;

	/* Loop: apos ficar parado tempo suficiente, volta ao ponto inicial. */
	if (cfg_.loop && (now_ms - last_move_ms_) >= cfg_.loop_ms) {
		sx_.set_target(start_x_);
		sy_.set_target(start_y_);
	}

	if (cfg_.smooth <= 0.001) {
		/* Sem suavizacao: segue o mouse exatamente. */
		sx_.reset(sx_.target());
		sy_.reset(sy_.target());
	} else {
		sx_.update(dt);
		sy_.update(dt);
	}

	/* Sway: inclina na direcao do movimento, amortecido pela velocidade. */
	const double vx = sx_.velocity();
	const double vy = sy_.velocity();
	const double speed = std::sqrt(vx * vx + vy * vy);
	double rot = 0.0;
	if (speed > 0.001)
		rot = clamp01(cfg_.sway) * std::atan2(vx, -vy);
	if (rot > 0.6)
		rot = 0.6;
	if (rot < -0.6)
		rot = -0.6;
	rotation_ = rot;
}

CursorRender CursorModel::render() const
{
	CursorRender out;
	out.x = sx_.value();
	out.y = sy_.value();
	out.size = cfg_.size;
	out.rotation = rotation_;

	if (!cfg_.enabled) {
		out.opacity = 0.0;
		out.visible = false;
		return out;
	}

	if (cfg_.hide_idle) {
		const double idle_for = last_update_ms_ - last_move_ms_;
		const double fade = clamp01((idle_for - cfg_.idle_delay_ms) / kIdleFadeMs);
		out.opacity = 1.0 - fade;
	} else {
		out.opacity = 1.0;
	}

	if (out.opacity < 0.0)
		out.opacity = 0.0;
	out.visible = out.opacity > 0.01;
	return out;
}

} // namespace topscreen::core
