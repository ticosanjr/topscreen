#pragma once

#include "types.hpp"

namespace topscreen::core {

/**
 * Area (in output pixels) where the captured screen is drawn.
 *
 * Determined by the padding (x/y) and, optionally, by "keep aspect".
 */
struct ContentArea {
	Vec2f origin; // top-left corner
	Vec2f size;   // width/height
};

ContentArea content_area(double target_w, double target_h, double pad_x, double pad_y, bool keep_aspect);

/**
 * DISPLAYED position (content uv) of a point of the SOURCE (source uv).
 *
 * With zoom `z` and focus `f`, a point `s` of the source appears at `f + (s - f) * z`
 * ("forward" zoom: what is far from the focus moves away from it).
 *
 * This is the inverse of the shader's sampling (`u = f + (c - f) / z`).
 */
Vec2f source_to_display(double s_x, double s_y, double focus_x, double focus_y, double zoom);

/** Output pixel of a source point (source -> display -> pixel). */
Vec2f source_to_pixel(double s_x, double s_y, double focus_x, double focus_y, double zoom,
		      const ContentArea &area);

/**
 * Inverse of source_to_display: a point on the displayed (zoomed) content back
 * to the source coordinate under it.
 *
 *     source = focus + (display - focus) / zoom
 */
Vec2f display_to_source(double d_x, double d_y, double focus_x, double focus_y, double zoom);

/** Inverse-zoom mapping for a rectangle (corners may be given in any order). */
void display_rect_to_source(double dx0, double dy0, double dx1, double dy1, double focus_x,
			    double focus_y, double zoom, double *sx0, double *sy0,
			    double *sx1, double *sy1);

/**
 * Cursor position in OUTPUT coordinates (0..1) for a pointer normalized to the
 * captured monitor (0..1).
 *
 * The cursor tip must point at the same captured pixel as the physical
 * pointer, so the pointer is mapped INTO the content area instead of being
 * clamped to it:
 *
 *     output = (area.origin + pointer * area.size) / target_size
 *
 * With wallpaper padding the tip therefore stops at the content edge, while
 * the icon body (drawn by the shader around this point) can overhang into the
 * padding. The pointer is clamped to 0..1 first, so leaving the monitor keeps
 * the tip on the content edge and never leaves the cursor entirely outside.
 */
Vec2f pointer_to_content_output(double nx, double ny, const ContentArea &area,
				double target_w, double target_h);

} // namespace topscreen::core
