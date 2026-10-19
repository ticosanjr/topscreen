#include "pen.hpp"

#include <algorithm>
#include <cmath>

namespace topscreen::core {

void PenCanvas::reset(double aspect, int max_dim, double thickness)
{
	aspect = aspect > 0.05 ? aspect : 16.0 / 9.0;
	max_dim = std::clamp(max_dim, 64, 2048);

	width_ = max_dim;
	height_ = std::max(32, static_cast<int>(std::lround(static_cast<double>(max_dim) / aspect)));
	/* Thickness is a fraction of the content height. */
	radius_px_ = std::max(0.75, thickness * static_cast<double>(height_) * 0.5);

	alpha_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), 0);
	++revision_;
	has_last_ = false;
}

void PenCanvas::add_point(Vec2f p)
{
	if (width_ <= 0 || height_ <= 0)
		return;

	if (!has_last_) {
		last_ = p;
		has_last_ = true;
		stamp_disc(p.x * width_, p.y * height_, radius_px_);
		return;
	}

	/* Ignore sub-pixel moves: keeps the mask stable and the work small. */
	const double dx = (p.x - last_.x) * width_;
	const double dy = (p.y - last_.y) * height_;
	if (dx * dx + dy * dy < 0.25)
		return;

	stamp_segment(last_, p);
	last_ = p;
}

void PenCanvas::set_points(const Vec2f *points, int count)
{
	if (!points || count <= 0)
		return;
	for (int i = 0; i < count; ++i)
		add_point(points[i]);
}

void PenCanvas::stamp_segment(Vec2f a, Vec2f b)
{
	const double x0 = a.x * width_;
	const double y0 = a.y * height_;
	const double x1 = b.x * width_;
	const double y1 = b.y * height_;

	const double dist = std::hypot(x1 - x0, y1 - y0);
	const int steps = std::max(1, static_cast<int>(dist / std::max(0.5, radius_px_ * 0.5)));
	for (int i = 1; i <= steps; ++i) {
		const double t = static_cast<double>(i) / steps;
		stamp_disc(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, radius_px_);
	}
}

void PenCanvas::stamp_disc(double cx, double cy, double radius)
{
	const int x0 = std::max(0, static_cast<int>(std::floor(cx - radius)));
	const int x1 = std::min(width_ - 1, static_cast<int>(std::ceil(cx + radius)));
	const int y0 = std::max(0, static_cast<int>(std::floor(cy - radius)));
	const int y1 = std::min(height_ - 1, static_cast<int>(std::ceil(cy + radius)));
	if (x0 > x1 || y0 > y1)
		return;

	const double r2 = radius * radius;
	for (int y = y0; y <= y1; ++y) {
		const double dy = y - cy;
		for (int x = x0; x <= x1; ++x) {
			const double dx = x - cx;
			const double d2 = dx * dx + dy * dy;
			if (d2 > r2)
				continue;
			/* Soft edge (1 px) so the stroke does not look pixelated. */
			const double edge = std::sqrt(d2) - (radius - 1.0);
			const double a = std::clamp(1.0 - std::max(0.0, edge), 0.0, 1.0);
			const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(width_) +
					   static_cast<size_t>(x);
			const uint8_t v = static_cast<uint8_t>(std::lround(a * 255.0));
			if (v > alpha_[idx])
				alpha_[idx] = v;
		}
	}
	++revision_;
}

} // namespace topscreen::core
