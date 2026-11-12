#include "mapping.hpp"

#include <algorithm>
#include <cmath>

namespace topscreen::core {

ContentArea content_area(double target_w, double target_h, double pad_x, double pad_y, bool keep_aspect)
{
	double w = target_w - pad_x * 2.0;
	double h = target_h - pad_y * 2.0;
	if (w < 1.0)
		w = 1.0;
	if (h < 1.0)
		h = 1.0;

	if (keep_aspect && target_w > 0.0 && target_h > 0.0) {
		const double sc = std::min(w / target_w, h / target_h);
		w = target_w * sc;
		h = target_h * sc;
	}

	ContentArea a;
	a.size.x = w;
	a.size.y = h;
	a.origin.x = (target_w - w) * 0.5;
	a.origin.y = (target_h - h) * 0.5;
	return a;
}

Vec2f source_to_display(double s_x, double s_y, double focus_x, double focus_y, double zoom)
{
	if (zoom < 1.0)
		zoom = 1.0;

	Vec2f d;
	d.x = focus_x + (s_x - focus_x) * zoom;
	d.y = focus_y + (s_y - focus_y) * zoom;
	return d;
}

Vec2f source_to_pixel(double s_x, double s_y, double focus_x, double focus_y, double zoom,
		      const ContentArea &area)
{
	const Vec2f d = source_to_display(s_x, s_y, focus_x, focus_y, zoom);
	Vec2f p;
	p.x = area.origin.x + d.x * area.size.x;
	p.y = area.origin.y + d.y * area.size.y;
	return p;
}

Vec2f display_to_source(double d_x, double d_y, double focus_x, double focus_y, double zoom)
{
	if (zoom < 1.0)
		zoom = 1.0;
	Vec2f out;
	out.x = focus_x + (d_x - focus_x) / zoom;
	out.y = focus_y + (d_y - focus_y) / zoom;
	return out;
}

void display_rect_to_source(double dx0, double dy0, double dx1, double dy1, double focus_x,
			    double focus_y, double zoom, double *sx0, double *sy0,
			    double *sx1, double *sy1)
{
	const Vec2f a = display_to_source(dx0, dy0, focus_x, focus_y, zoom);
	const Vec2f b = display_to_source(dx1, dy1, focus_x, focus_y, zoom);
	if (sx0)
		*sx0 = a.x < b.x ? a.x : b.x;
	if (sy0)
		*sy0 = a.y < b.y ? a.y : b.y;
	if (sx1)
		*sx1 = a.x < b.x ? b.x : a.x;
	if (sy1)
		*sy1 = a.y < b.y ? b.y : a.y;
}

Vec2f pointer_to_content_output(double nx, double ny, const ContentArea &area,
				double target_w, double target_h)
{
	const double px = std::clamp(nx, 0.0, 1.0);
	const double py = std::clamp(ny, 0.0, 1.0);
	const double w = target_w > 0.0 ? target_w : 1.0;
	const double h = target_h > 0.0 ? target_h : 1.0;

	Vec2f out;
	out.x = (area.origin.x + px * area.size.x) / w;
	out.y = (area.origin.y + py * area.size.y) / h;
	return out;
}

} // namespace topscreen::core
