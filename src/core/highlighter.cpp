#include "highlighter.hpp"

#include <algorithm>
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
	if (mode_draws_line(cfg_.mode) && trail_count_ < kMaxPoints) {
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

	if (!mode_draws_line(cfg_.mode) || trail_count_ >= kMaxPoints)
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

	/* Zoom gesture: remember the selection so the filter can zoom into it.
	 * The hold path (update) fires while the button is still down; this path
	 * only handles a release that happened before the trigger, with a real
	 * drag behind it; a quick click must do nothing. */
	if (!zoom_fired_ && mode_wants_zoom(cfg_.mode) && cfg_.enabled) {
		double rx0 = 0.0, ry0 = 0.0, rx1 = 0.0, ry1 = 0.0;
		selection_rect(&rx0, &ry0, &rx1, &ry1);
		if (rx1 - rx0 >= cfg_.min_span && ry1 - ry0 >= cfg_.min_span) {
			zx0_ = rx0;
			zy0_ = ry0;
			zx1_ = rx1;
			zy1_ = ry1;
			zoom_request_ = true;
			/* Pure zoom draws nothing; the other zoom modes keep their mark
			 * (box or line) visible while the view glides back. */
			if (cfg_.mode == static_cast<int>(HighlightMode::Zoom))
				has_ = false;
		}
	}
}

/* Selection the zoom gesture works on. For the line modes it is the bounding
 * box of the whole stroke, because a freehand shape (a circle) ends where it
 * started: the start/end rectangle would be degenerate and the gesture would
 * zoom into a point instead of the drawn area. For every other mode it is the
 * dragged rectangle. */
void HighlighterModel::selection_rect(double *x0, double *y0, double *x1, double *y1) const
{
	if (mode_draws_line(cfg_.mode) && trail_count_ > 0) {
		double minx = trail_[0].x, maxx = minx;
		double miny = trail_[0].y, maxy = miny;
		for (int i = 1; i < trail_count_; ++i) {
			const double px = trail_[i].x;
			const double py = trail_[i].y;
			minx = minx < px ? minx : px;
			maxx = maxx > px ? maxx : px;
			miny = miny < py ? miny : py;
			maxy = maxy > py ? maxy : py;
		}
		*x0 = minx;
		*y0 = miny;
		*x1 = maxx;
		*y1 = maxy;
		return;
	}
	*x0 = sx_ < ex_ ? sx_ : ex_;
	*y0 = sy_ < ey_ ? sy_ : ey_;
	*x1 = sx_ < ex_ ? ex_ : sx_;
	*y1 = sy_ < ey_ ? ey_ : sy_;
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

void HighlighterModel::update(double now)
{
	if (mode_wants_zoom(cfg_.mode) && cfg_.enabled && dragging_ && !zoom_fired_ &&
	    now - press_ms_ >= cfg_.zoom_delay_ms) {
		/* The drawn area always wins over the "just held" point; for the
		 * line modes that area is the stroke's bounding box. */
		double rx0 = 0.0, ry0 = 0.0, rx1 = 0.0, ry1 = 0.0;
		selection_rect(&rx0, &ry0, &rx1, &ry1);
		if (rx1 - rx0 >= cfg_.min_span && ry1 - ry0 >= cfg_.min_span) {
			zx0_ = rx0;
			zy0_ = ry0;
			zx1_ = rx1;
			zy1_ = ry1;
		} else {
			zx0_ = zx1_ = ex_;
			zy0_ = zy1_ = ey_;
		}
		zoom_request_ = true;
		zoom_fired_ = true;
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
	const bool is_line = mode_draws_line(cfg_.mode);
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

	/* Pure zoom modes never paint; BoxZoom draws the box and LineZoom the line
	 * below, while the filter animates the zoom. */
	if (cfg_.mode == static_cast<int>(HighlightMode::Zoom) ||
	    cfg_.mode == static_cast<int>(HighlightMode::Zoom)) {
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
	if (mode_draws_line(cfg_.mode)) {
		out.points = trail_;
		out.point_count = trail_count_;
	}
	return out;
}

int subsample_line(const Vec2f *points, int count, Vec2f *out, int max_out)
{
	if (!points || !out || count <= 0 || max_out <= 0)
		return 0;
	if (count <= max_out) {
		for (int i = 0; i < count; ++i)
			out[i] = points[i];
		return count;
	}
	if (max_out == 1) {
		out[0] = points[0];
		return 1;
	}

	/* Arc-length resampling: the raw trail arrives at the pointer polling
	 * rate, so slow and fast parts have very different point density.
	 * Sampling equal distances (instead of equal indexes) keeps the shader
	 * segments even as the stroke changes speed. */
	double total = 0.0;
	for (int i = 1; i < count; ++i) {
		const double dx = points[i].x - points[i - 1].x;
		const double dy = points[i].y - points[i - 1].y;
		total += std::sqrt(dx * dx + dy * dy);
	}
	if (total <= 1e-12) {
		out[0] = points[0];
		return 1;
	}

	out[0] = points[0];
	out[max_out - 1] = points[count - 1];

	int seg = 1;
	double seg_start = 0.0;
	double dx = points[1].x - points[0].x;
	double dy = points[1].y - points[0].y;
	double seg_len = std::sqrt(dx * dx + dy * dy);

	for (int i = 1; i < max_out - 1; ++i) {
		const double target = total * static_cast<double>(i) / (max_out - 1);
		while (seg < count - 1 && seg_start + seg_len < target) {
			seg_start += seg_len;
			++seg;
			dx = points[seg].x - points[seg - 1].x;
			dy = points[seg].y - points[seg - 1].y;
			seg_len = std::sqrt(dx * dx + dy * dy);
		}
		if (seg_len <= 1e-12) {
			out[i] = points[seg];
			continue;
		}
		const double t =
			std::clamp((target - seg_start) / seg_len, 0.0, 1.0);
		out[i].x = points[seg - 1].x + (points[seg].x - points[seg - 1].x) * t;
		out[i].y = points[seg - 1].y + (points[seg].y - points[seg - 1].y) * t;
	}
	return max_out;
}

} // namespace topscreen::core
