#include "geometry.hpp"

#include <cmath>

namespace topscreen::core {

double sd_round_box(double px, double py, double hx, double hy, double radius)
{
	const double qx = std::fabs(px) - hx + radius;
	const double qy = std::fabs(py) - hy + radius;
	const double mx = qx > 0.0 ? qx : 0.0;
	const double my = qy > 0.0 ? qy : 0.0;
	return std::fmin(std::fmax(qx, qy), 0.0) + std::sqrt(mx * mx + my * my) - radius;
}

double rounded_rect_mask(double px, double py, double hx, double hy, double radius, double aa)
{
	const double d = sd_round_box(px, py, hx, hy, radius);
	return 1.0 - smoothstep(-aa, aa, d);
}

Vec2f zoom_uv(double content_u, double content_v, double focus_x, double focus_y, double zoom)
{
	if (zoom < 0.0001)
		zoom = 0.0001;
	Vec2f out;
	out.x = focus_x + (content_u - focus_x) / zoom;
	out.y = focus_y + (content_v - focus_y) / zoom;
	return out;
}

Vec2f pixel_to_content(double px, double py, double target_w, double target_h, double pad)
{
	Vec2f out;
	const double cw = target_w - pad * 2.0;
	const double ch = target_h - pad * 2.0;
	out.x = cw > 0.0 ? (px - pad) / cw : 0.0;
	out.y = ch > 0.0 ? (py - pad) / ch : 0.0;
	return out;
}

double clamp_focus(double focus, double zoom)
{
	if (zoom <= 1.0)
		return 0.5;
	double margin = (zoom - 1.0) / (2.0 * zoom);
	if (margin < 0.0)
		margin = 0.0;
	if (margin > 0.5)
		margin = 0.5;
	if (focus < margin)
		focus = margin;
	if (focus > 1.0 - margin)
		focus = 1.0 - margin;
	return focus;
}

} // namespace topscreen::core
