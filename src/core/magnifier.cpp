#include "magnifier.hpp"

#include <algorithm>
#include <cmath>

#include "core/easing.hpp"

namespace topscreen::core {

void MagnifierModel::configure(const MagnifierConfig &cfg)
{
	cfg_ = cfg;
	if (!initialized_) {
		initialized_ = true;
		target_ = cfg_.enabled;
		amount_ = target_ ? 1.0 : 0.0;
		from_ = amount_;
		to_ = amount_;
		return;
	}
	if (cfg_.enabled != target_)
		begin_transition(cfg_.enabled, last_ms_);
}

void MagnifierModel::set_active(bool active, double now_ms)
{
	last_ms_ = now_ms;
	if (!initialized_) {
		initialized_ = true;
		target_ = active;
		amount_ = active ? 1.0 : 0.0;
		from_ = amount_;
		to_ = amount_;
		return;
	}
	if (active == target_)
		return;
	begin_transition(active, now_ms);
}

void MagnifierModel::begin_transition(bool active, double now_ms)
{
	target_ = active;
	from_ = amount_;
	to_ = active ? 1.0 : 0.0;
	start_ms_ = now_ms;
	duration_ms_ = std::max(0.0, active ? cfg_.in_ms : cfg_.out_ms);
	last_ms_ = now_ms;
	if (duration_ms_ <= 0.0)
		amount_ = to_;
}

void MagnifierModel::update_focus(double cursor_x, double cursor_y, double aspect)
{
	const double cx = std::clamp(cursor_x, 0.0, 1.0);
	const double cy = std::clamp(cursor_y, 0.0, 1.0);

	if (!cfg_.follow_cursor) {
		focus_x_ = std::clamp(cfg_.center_x, 0.0, 1.0);
		focus_y_ = std::clamp(cfg_.center_y, 0.0, 1.0);
		sample_x_ = focus_x_;
		sample_y_ = focus_y_;
		return;
	}

	/* What the lens shows follows the cursor all the way to the edge. */
	sample_x_ = cx;
	sample_y_ = cy;

	/* The lens canvas stops before it would leave the screen. The radius is a
	 * fraction of the shorter target side, so the normalized radius depends
	 * on the target aspect. */
	const double a = aspect > 0.0 && std::isfinite(aspect) ? aspect : 1.0;
	const double r = std::max(cfg_.radius, 0.001);
	const double rx = r * std::min(1.0, 1.0 / a);
	const double ry = r * std::min(1.0, a);
	const double lo_x = std::min(rx, 0.5);
	const double hi_x = std::max(1.0 - rx, 0.5);
	const double lo_y = std::min(ry, 0.5);
	const double hi_y = std::max(1.0 - ry, 0.5);
	focus_x_ = std::clamp(cx, lo_x, hi_x);
	focus_y_ = std::clamp(cy, lo_y, hi_y);
}

void MagnifierModel::update(double now_ms, double cursor_x, double cursor_y, double aspect)
{
	last_ms_ = now_ms;
	update_focus(cursor_x, cursor_y, aspect);

	if (duration_ms_ <= 0.0)
		return;

	const double t = std::clamp((now_ms - start_ms_) / duration_ms_, 0.0, 1.0);
	const double e = ease_in_out_cubic(t);
	amount_ = from_ + (to_ - from_) * e;
	if (t >= 1.0)
		duration_ms_ = 0.0;
}

MagnifierRender MagnifierModel::render() const
{
	MagnifierRender out;
	out.amount = std::clamp(amount_, 0.0, 1.0);
	const double zoom = std::max(1.0, cfg_.zoom_amount);
	out.zoom = 1.0 + (zoom - 1.0) * out.amount;
	out.focus_x = focus_x_;
	out.focus_y = focus_y_;
	out.sample_x = sample_x_;
	out.sample_y = sample_y_;
	out.drop = 0.0;
	return out;
}

} // namespace topscreen::core
