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
	pointer_x_ = x;
	pointer_y_ = y;
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

	if (std::fabs(x - pointer_x_) > 1e-6 || std::fabs(y - pointer_y_) > 1e-6) {
		pointer_x_ = x;
		pointer_y_ = y;
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
	const double dx = glide_to_x_ - glide_from_x_;
	const double dy = glide_to_y_ - glide_from_y_;
	const double dist = std::sqrt(dx * dx + dy * dy);
	if (dist > 1e-6) {
		const double curve = dist * 0.06;
		glide_ctrl_x_ = (glide_from_x_ + glide_to_x_) * 0.5 - dy / dist * curve;
		glide_ctrl_y_ = (glide_from_y_ + glide_to_y_) * 0.5 + dx / dist * curve;
	} else {
		glide_ctrl_x_ = glide_to_x_;
		glide_ctrl_y_ = glide_to_y_;
	}
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
	const double u = 1.0 - e;
	const double x = u * u * glide_from_x_ + 2.0 * u * e * glide_ctrl_x_ +
			 e * e * glide_to_x_;
	const double y = u * u * glide_from_y_ + 2.0 * u * e * glide_ctrl_y_ +
			 e * e * glide_to_y_;
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

	if (cfg_.follow_mode == static_cast<int>(CursorFollowMode::Exact)) {
		sx_.reset(target_x_);
		sy_.reset(target_y_);
		glide_active_ = false;
	} else {
		/* Only Glide A→B remains; legacy Spring/Sampled values were mapped
		 * to this mode by config sanitization. */
		if (!glide_active_ && target_dirty_) {
			start_glide(now_ms);
			target_dirty_ = false;
		}
		step_glide(now_ms);
	}

	/* Bitmap cursors use a fixed 30° clockwise tilt. */
	rotation_ = 0.5235987755982988;

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
