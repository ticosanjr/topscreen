#include "highlighter.hpp"

#include <cmath>

namespace topscreen::core {

void HighlighterModel::configure(const HighlighterConfig &cfg)
{
	cfg_ = cfg;
}

void HighlighterModel::begin(double x, double y, double now)
{
	(void)now;
	sx_ = ex_ = x;
	sy_ = ey_ = y;
	dragging_ = true;
	has_ = true;
	alpha_ = 1.0;
}

void HighlighterModel::move(double x, double y, double now)
{
	(void)now;
	if (!dragging_)
		return;
	ex_ = x;
	ey_ = y;
}

void HighlighterModel::end(double now)
{
	if (!dragging_)
		return;
	dragging_ = false;
	ended_ms_ = now;
}

void HighlighterModel::update(double now)
{
	if (!has_) {
		alpha_ = 0.0;
		return;
	}

	if (dragging_) {
		alpha_ = 1.0;
		return;
	}

	const double elapsed = now - ended_ms_;
	if (elapsed <= cfg_.hold_ms) {
		alpha_ = 1.0;
		return;
	}

	const double fade = cfg_.fade_ms > 1.0 ? cfg_.fade_ms : 1.0;
	alpha_ = 1.0 - (elapsed - cfg_.hold_ms) / fade;
	if (alpha_ <= 0.0) {
		alpha_ = 0.0;
		has_ = false;
	}
}

HighlightRender HighlighterModel::render() const
{
	HighlightRender out;

	const double dx = std::fabs(ex_ - sx_);
	const double dy = std::fabs(ey_ - sy_);
	const bool big_enough = dx >= cfg_.min_span && dy >= cfg_.min_span;

	if (!cfg_.enabled || !has_ || alpha_ <= 0.01 || !big_enough) {
		out.active = false;
		return out;
	}

	out.active = true;
	out.x0 = sx_ < ex_ ? sx_ : ex_;
	out.y0 = sy_ < ey_ ? sy_ : ey_;
	out.x1 = sx_ < ex_ ? ex_ : sx_;
	out.y1 = sy_ < ey_ ? ey_ : sy_;
	out.alpha = alpha_;
	out.color = cfg_.color;
	return out;
}

} // namespace topscreen::core
