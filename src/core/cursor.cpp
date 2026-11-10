#include "cursor.hpp"

#include <algorithm>
#include <cmath>

#include "cursor_catalog.hpp"
#include "easing.hpp"
#include "math.hpp"

namespace topscreen::core {

namespace {

constexpr double kIdleFadeMs = 300.0;
constexpr double kPi = 3.14159265358979323846;

double glide_duration(double ms)
{
	return ms >= 1.0 ? ms : 1.0;
}

} // namespace

void CursorModel::configure(const CursorConfig &cfg)
{
	const int previous_mode = cfg_.follow_mode;
	cfg_ = cfg;
	switch (cfg_.follow_mode) {
	case static_cast<int>(CursorFollowMode::Exact):
	case static_cast<int>(CursorFollowMode::GlideAB):
	case static_cast<int>(CursorFollowMode::Smooth):
		break;
	default:
		cfg_.follow_mode = static_cast<int>(CursorFollowMode::GlideAB);
		break;
	}
	if (cfg_.follow_mode != previous_mode) {
		glide_active_ = false;
		target_dirty_ = true;
	}
	cfg_.glide_ms = glide_duration(cfg_.glide_ms);
	cfg_.smooth_ms = cfg_.smooth_ms >= 1.0 ? cfg_.smooth_ms : 1.0;
	resolved_style_ = cursor_style_resolve(cfg_.style);
	resolved_link_style_ =
		cfg_.secondary_style >= 0 ? cursor_style_resolve(cfg_.secondary_style) : -1;
	style_rotation_ = cursor_style_keeps_tilt(resolved_style_) ? 30.0 : 0.0;
	link_rotation_ =
		resolved_link_style_ >= 0 && cursor_style_keeps_tilt(resolved_link_style_) ? 30.0
										      : 0.0;
}

void CursorModel::reset(double x, double y, double now_ms)
{
	x_ = x;
	y_ = y;
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
	glide_from_x_ = x_;
	glide_from_y_ = y_;
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
	x_ = x;
	y_ = y;

	if (t >= 1.0) {
		x_ = glide_to_x_;
		y_ = glide_to_y_;
		glide_active_ = false;
	}
}

void CursorModel::update(double now_ms)
{
	if (!started_)
		return;

	const double dt = std::max(0.0, (now_ms - last_update_ms_) / 1000.0);
	last_update_ms_ = now_ms;

	if (cfg_.follow_mode == static_cast<int>(CursorFollowMode::Exact)) {
		x_ = target_x_;
		y_ = target_y_;
		glide_active_ = false;
	} else if (cfg_.follow_mode == static_cast<int>(CursorFollowMode::Smooth)) {
		const double tau = std::max(0.001, cfg_.smooth_ms / 1000.0);
		const double alpha = dt > 0.0 ? 1.0 - std::exp(-dt / tau) : 0.0;
		x_ += (target_x_ - x_) * alpha;
		y_ += (target_y_ - y_) * alpha;
		glide_active_ = false;
	} else {
		/* Glide A→B; legacy Spring/Sampled values were mapped to this mode
		 * by config sanitization. */
		if (!glide_active_ && target_dirty_) {
			start_glide(now_ms);
			target_dirty_ = false;
		}
		step_glide(now_ms);
	}
}

CursorRender CursorModel::render() const
{
	CursorRender out;
	out.x = x_;
	out.y = y_;
	out.size = cfg_.size;
	const bool use_link = cfg_.link_detection && link_ && resolved_link_style_ >= 0;
	out.style = use_link ? resolved_link_style_ : resolved_style_;

	/* Bundled cursors (Bibata through Dot) are drawn upright; only the
	 * user-supplied Custom images keep the fixed 30° tilt. The profile can
	 * add an extra rotation offset on top of that baseline. */
	const double base_deg = use_link ? link_rotation_ : style_rotation_;
	out.rotation = (base_deg + cfg_.rotation_offset_deg) * kPi / 180.0;

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
