#include "cursor.hpp"

#include <algorithm>
#include <cmath>

#include "cursor_catalog.hpp"
#include "easing.hpp"
#include "math.hpp"

namespace topscreen::core {

namespace {

constexpr double kIdleFadeMs = 300.0;

double glide_duration(double ms)
{
	return ms >= 1.0 ? ms : 1.0;
}

} // namespace

double CursorModel::stiffness() const
{
	/* smooth 0 -> soft spring, smooth 1 -> stiff spring. */
	const double s = clamp01(cfg_.smooth);
	return 20.0 + s * 380.0;
}

void CursorModel::configure(const CursorConfig &cfg)
{
	const int previous_mode = cfg_.follow_mode;
	cfg_ = cfg;
	cfg_.follow_mode = std::clamp(cfg_.follow_mode, 0, 3);
	if (cfg_.follow_mode != previous_mode) {
		glide_active_ = false;
		target_dirty_ = true;
	}
	cfg_.glide_ms = glide_duration(cfg_.glide_ms);
	cfg_.glide_sample_ms = glide_duration(cfg_.glide_sample_ms);
	sx_.set_stiffness(stiffness());
	sy_.set_stiffness(stiffness());
}

void CursorModel::reset(double x, double y, double now_ms)
{
	sx_.reset(x);
	sy_.reset(y);
	target_x_ = x;
	target_y_ = y;
	start_x_ = x;
	start_y_ = y;
	glide_from_x_ = x;
	glide_from_y_ = y;
	glide_to_x_ = x;
	glide_to_y_ = y;
	glide_active_ = false;
	target_dirty_ = false;
	next_sample_ms_ = now_ms;
	rotation_ = 0.0;
	last_move_ms_ = now_ms;
	last_update_ms_ = now_ms;
	started_ = true;
}

void CursorModel::set_target(double x, double y, double now_ms)
{
	if (!started_)
		reset(x, y, now_ms);

	if (std::fabs(x - target_x_) > 1e-6 || std::fabs(y - target_y_) > 1e-6) {
		target_x_ = x;
		target_y_ = y;
		target_dirty_ = true;
		last_move_ms_ = now_ms;
	}
}

void CursorModel::start_glide(double now_ms)
{
	glide_from_x_ = sx_.value();
	glide_from_y_ = sy_.value();
	glide_to_x_ = target_x_;
	glide_to_y_ = target_y_;
	glide_start_ms_ = now_ms;
	glide_end_ms_ = now_ms + glide_duration(cfg_.glide_ms);
	glide_active_ = true;
}

void CursorModel::step_glide(double now_ms)
{
	if (!glide_active_)
		return;

	const double span = std::max(1.0, glide_end_ms_ - glide_start_ms_);
	const double t = std::clamp((now_ms - glide_start_ms_) / span, 0.0, 1.0);
	const double e = ease_in_out_cubic(t);
	const double x = glide_from_x_ + (glide_to_x_ - glide_from_x_) * e;
	const double y = glide_from_y_ + (glide_to_y_ - glide_from_y_) * e;
	sx_.reset(x);
	sy_.reset(y);

	if (t >= 1.0) {
		sx_.reset(glide_to_x_);
		sy_.reset(glide_to_y_);
		glide_active_ = false;
	}
}

void CursorModel::update(double now_ms)
{
	if (!started_)
		return;

	double dt = (now_ms - last_update_ms_) / 1000.0;
	last_update_ms_ = now_ms;
	if (dt < 0.0)
		dt = 0.0;

	/* Loop: after being idle long enough, returns to the starting point. */
	if (cfg_.loop && (now_ms - last_move_ms_) >= cfg_.loop_ms &&
	    (std::fabs(target_x_ - start_x_) > 1e-9 || std::fabs(target_y_ - start_y_) > 1e-9)) {
		target_x_ = start_x_;
		target_y_ = start_y_;
		target_dirty_ = true;
	}

	switch (cfg_.follow_mode) {
	case static_cast<int>(CursorFollowMode::Exact):
		sx_.reset(target_x_);
		sy_.reset(target_y_);
		glide_active_ = false;
		break;

	case static_cast<int>(CursorFollowMode::GlideSampled):
		if (!glide_active_ && target_dirty_ && now_ms >= next_sample_ms_) {
			start_glide(now_ms);
			target_dirty_ = false;
			next_sample_ms_ = now_ms + glide_duration(cfg_.glide_sample_ms);
		}
		step_glide(now_ms);
		break;

	case static_cast<int>(CursorFollowMode::GlideAB):
		if (!glide_active_ && target_dirty_) {
			start_glide(now_ms);
			target_dirty_ = false;
		}
		step_glide(now_ms);
		break;

	case static_cast<int>(CursorFollowMode::Spring):
	default:
		sx_.set_target(target_x_);
		sy_.set_target(target_y_);
		if (cfg_.smooth <= 0.001) {
			/* No smoothing: follows the mouse exactly. */
			sx_.reset(target_x_);
			sy_.reset(target_y_);
		} else {
			sx_.update(dt);
			sy_.update(dt);
		}
		break;
	}

	/* Sway: tilts toward the movement direction, damped by speed. */
	if (cfg_.sway > 0.0005) {
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
	} else {
		rotation_ = 0.0;
	}
}

CursorRender CursorModel::render() const
{
	CursorRender out;
	out.x = sx_.value();
	out.y = sy_.value();
	out.size = cfg_.size;
	out.rotation = rotation_;
	out.style = cursor_style_resolve(cfg_.style);

	if (cfg_.link_detection && link_ && cfg_.secondary_style >= 0)
		out.style = cursor_style_resolve(cfg_.secondary_style);

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
