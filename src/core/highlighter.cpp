#include "highlighter.hpp"

#include <cmath>

#include "math.hpp"

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
	zoom_request_ = false;
	zoom_fired_ = false;
	press_ms_ = now;

	trail_count_ = 0;
	if (cfg_.mode == static_cast<int>(HighlightMode::Line) && trail_count_ < kMaxPoints) {
		trail_[trail_count_].x = static_cast<float>(x);
		trail_[trail_count_].y = static_cast<float>(y);
		++trail_count_;
	}
}

void HighlighterModel::move(double x, double y, double now)
{
	(void)now;
	if (!dragging_)
		return;
	ex_ = x;
	ey_ = y;

	if (cfg_.mode != static_cast<int>(HighlightMode::Line) || trail_count_ >= kMaxPoints)
		return;

	/* Skip points that are too close: keeps the stroke smooth and the buffer
	 * small without visible loss. */
	const double dx = x - trail_[trail_count_ - 1].x;
	const double dy = y - trail_[trail_count_ - 1].y;
	if (dx * dx + dy * dy < 0.00002)
		return;

	trail_[trail_count_].x = static_cast<float>(x);
	trail_[trail_count_].y = static_cast<float>(y);
	++trail_count_;
}

void HighlighterModel::end(double now)
{
	if (!dragging_)
		return;
	dragging_ = false;
	ended_ms_ = now;

	const double dx = std::fabs(ex_ - sx_);
	const double dy = std::fabs(ey_ - sy_);
	const bool big_enough = dx >= cfg_.min_span && dy >= cfg_.min_span;

	/* Zoom gesture: remember the selection so the filter can zoom into it.
	 * Nothing is drawn. */
	/* Zoom gesture. The hold path (update) fires while the button is still
	 * down; this path only handles a release that happened before the
	 * trigger, with a real drag behind it. */
	if (!zoom_fired_ && cfg_.mode == static_cast<int>(HighlightMode::Zoom) && cfg_.enabled &&
	    big_enough) {
		zx0_ = sx_ < ex_ ? sx_ : ex_;
		zy0_ = sy_ < ey_ ? sy_ : ey_;
		zx1_ = sx_ < ex_ ? ex_ : sx_;
		zy1_ = sy_ < ey_ ? ey_ : sy_;
		zoom_request_ = true;
		has_ = false;
	}
}

bool HighlighterModel::take_zoom_request(double *x0, double *y0, double *x1, double *y1)
{
	if (!zoom_request_)
		return false;
	zoom_request_ = false;
	if (x0)
		*x0 = zx0_;
	if (y0)
		*y0 = zy0_;
	if (x1)
		*x1 = zx1_;
	if (y1)
		*y1 = zy1_;
	return true;
}

/* Squared distance that counts as "started dragging" for punch modes. */
static constexpr double kPunchDeadZone2 = 0.003 * 0.003;

void HighlighterModel::update(double now)
{
	if (cfg_.mode == static_cast<int>(HighlightMode::Zoom) && cfg_.enabled && dragging_ &&
	    !zoom_fired_) {
		const bool punch = cfg_.zoom_dir >= 2;
		const double dx = ex_ - sx_;
		const double dy = ey_ - sy_;
		const bool moved = dx * dx + dy * dy > kPunchDeadZone2;
		const bool timeout = now - press_ms_ >= cfg_.zoom_delay_ms;
		if ((punch && moved) || (!punch && timeout)) {
			/* Point zoom when the button was just held; the drawn rect (if
			 * any) always wins. */
			const double rdx = std::fabs(ex_ - sx_);
			const double rdy = std::fabs(ey_ - sy_);
			if (rdx >= cfg_.min_span && rdy >= cfg_.min_span) {
				zx0_ = sx_ < ex_ ? sx_ : ex_;
				zy0_ = sy_ < ey_ ? sy_ : ey_;
				zx1_ = sx_ < ex_ ? ex_ : sx_;
				zy1_ = sy_ < ey_ ? ey_ : sy_;
			} else {
				zx0_ = zx1_ = ex_;
				zy0_ = zy1_ = ey_;
			}
			zoom_request_ = true;
			zoom_fired_ = true;
		}
	}

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

	/* The line is a freehand stroke: it only needs a trail (two points or
	 * more). The filled box still needs a real drag, so plain clicks never
	 * leave a mark behind. */
	const bool is_line = cfg_.mode == static_cast<int>(HighlightMode::Line);
	if (is_line) {
		if (!cfg_.enabled || !has_ || alpha_ <= 0.01 || trail_count_ < 2) {
			out.active = false;
			return out;
		}
	} else {
		const double dx = std::fabs(ex_ - sx_);
		const double dy = std::fabs(ey_ - sy_);
		if (!cfg_.enabled || !has_ || alpha_ <= 0.01 || dx < cfg_.min_span ||
		    dy < cfg_.min_span) {
			out.active = false;
			return out;
		}
	}

	/* Zoom mode never paints; the filter animates the zoom instead. */
	if (cfg_.mode == static_cast<int>(HighlightMode::Zoom)) {
		out.active = false;
		return out;
	}

	out.active = true;
	out.mode = cfg_.mode;
	out.x0 = sx_ < ex_ ? sx_ : ex_;
	out.y0 = sy_ < ey_ ? sy_ : ey_;
	out.x1 = sx_ < ex_ ? ex_ : sx_;
	out.y1 = sy_ < ey_ ? ey_ : sy_;
	out.thickness = clamp01(cfg_.thickness);
	/* User ceiling: 0 hides the mark, 1 keeps the legacy full strength. */
	out.alpha = alpha_ * clamp01(cfg_.opacity);
	out.color = cfg_.color;
	if (cfg_.mode == static_cast<int>(HighlightMode::Line)) {
		out.points = trail_;
		out.point_count = trail_count_;
	}
	return out;
}

} // namespace topscreen::core
