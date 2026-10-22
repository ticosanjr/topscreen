#include "magnifier.hpp"

#include <algorithm>

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

void MagnifierModel::update_focus(double cursor_x, double cursor_y)
{
	if (cfg_.follow_cursor) {
		cursor_x_ = std::clamp(cursor_x, 0.0, 1.0);
		cursor_y_ = std::clamp(cursor_y, 0.0, 1.0);
	} else {
		cursor_x_ = std::clamp(cfg_.center_x, 0.0, 1.0);
		cursor_y_ = std::clamp(cfg_.center_y, 0.0, 1.0);
	}
}

void MagnifierModel::update(double now_ms, double cursor_x, double cursor_y)
{
	last_ms_ = now_ms;
	update_focus(cursor_x, cursor_y);

	if (duration_ms_ <= 0.0)
		return;

	const double t = std::clamp((now_ms - start_ms_) / duration_ms_, 0.0, 1.0);
	const double e = target_ ? ease_out_cubic(t) : ease_in_cubic(t);
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
	out.focus_x = cursor_x_;
	out.focus_y = cursor_y_;
	const double direction = target_ ? -1.0 : 1.0;
	out.drop = (!target_ && out.amount <= 0.001)
			   ? 0.0
			   : (1.0 - out.amount) * cfg_.drop_px * direction;
	return out;
}

} // namespace topscreen::core
