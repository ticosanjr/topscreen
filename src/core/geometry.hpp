#pragma once

#include "types.hpp"

namespace topscreen::core {

/** smoothstep in C++ (mirrors the shader's). */
inline double smoothstep(double edge0, double edge1, double x)
{
	if (edge0 == edge1)
		return x < edge0 ? 0.0 : 1.0;
	double t = (x - edge0) / (edge1 - edge0);
	if (t < 0.0)
		t = 0.0;
	if (t > 1.0)
		t = 1.0;
	return t * t * (3.0 - 2.0 * t);
}

/**
 * Signed distance to a rounded rectangle centered at the origin.
 * Negative inside, positive outside. Mirrors the shader's sd_round_box().
 */
double sd_round_box(double px, double py, double hx, double hy, double radius);

/**
 * Coverage mask of a rounded rectangle (0 outside, 1 inside),
 * with anti-aliasing in a band of width `aa` around the edge.
 * Mirrors the shader's calculation to allow testing.
 */
double rounded_rect_mask(double px, double py, double hx, double hy, double radius, double aa);

/**
 * UV transformation of the zoom/pan.
 * content_uv in 0..1 -> sampling UV of the screen (0..1).
 */
Vec2f zoom_uv(double content_u, double content_v, double focus_x, double focus_y, double zoom);

/**
 * Converts an output pixel into a content coordinate (0..1), taking the
 * internal padding into account. Outside the content it produces values outside 0..1.
 */
Vec2f pixel_to_content(double px, double py, double target_w, double target_h, double pad);

/**
 * Clamps the zoom focus so it does not sample outside the screen (0..1).
 *
 * With focus in [0,1] the visible window (width 1/zoom) always fits in the source.
 */
double clamp_focus(double focus, double zoom);

/** Focus range that keeps a `source_pos` point visible (0..1). */
struct FocusLimits {
	double lo = 0.0;
	double hi = 1.0;
};

FocusLimits focus_limits(double source_pos, double zoom);

/** Clamps the focus keeping the `source_pos` point inside the screen. */
double clamp_focus_keep(double source_pos, double focus, double zoom);

/**
 * Focus that places the source point `source_pos` at the CENTER of the screen.
 *
 * Solves `0.5 = f + (s - f) * z` for `f`, with the result in [0,1]:
 *   f = (s*z - 0.5) / (z - 1)
 *
 * With this the view anticipates the edges (the corner appears before the mouse
 * reaches it) and the cursor stays visible all the time.
 */
double center_focus_for(double source_pos, double zoom);

} // namespace topscreen::core
