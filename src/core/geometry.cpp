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
	(void)zoom;
	/*
	 * Com foco em [0,1] a janela visivel (1/zoom) sempre cabe na fonte.
	 * Nao ha margem extra: exigir margem era o que impedia chegar aos cantos.
	 */
	if (focus < 0.0)
		focus = 0.0;
	if (focus > 1.0)
		focus = 1.0;
	return focus;
}

FocusLimits focus_limits(double source_pos, double zoom)
{
	FocusLimits l;
	if (zoom <= 1.0001) {
		l.lo = 0.0;
		l.hi = 1.0;
		return l;
	}

	/*
	 * c = foco + (fonte - foco) * zoom  deve ficar em [0,1]:
	 *   foco <= fonte*zoom/(zoom-1)   e   foco >= (fonte*zoom - 1)/(zoom-1)
	 */
	const double z = zoom - 1.0;
	l.lo = (source_pos * zoom - 1.0) / z;
	l.hi = (source_pos * zoom) / z;

	if (l.lo < 0.0)
		l.lo = 0.0;
	if (l.hi > 1.0)
		l.hi = 1.0;
	if (l.lo > l.hi)
		l.lo = l.hi = 0.5;
	return l;
}

double clamp_focus_keep(double source_pos, double focus, double zoom)
{
	const FocusLimits l = focus_limits(source_pos, zoom);
	if (focus < l.lo)
		return l.lo;
	if (focus > l.hi)
		return l.hi;
	return focus;
}

double center_focus_for(double source_pos, double zoom)
{
	if (zoom <= 1.0001)
		return 0.5;

	const double f = (source_pos * zoom - 0.5) / (zoom - 1.0);
	return clamp_focus(f, zoom);
}

} // namespace topscreen::core
