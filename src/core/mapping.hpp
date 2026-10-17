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

} // namespace topscreen::core
