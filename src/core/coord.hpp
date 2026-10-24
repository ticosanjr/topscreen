#pragma once

#include "types.hpp"

namespace topscreen::core {

/** Rectangle of a monitor/screen in global coordinates. */
struct Monitor {
	double x = 0.0;
	double y = 0.0;
	double width = 1920.0;
	double height = 1080.0;
};

/** Converts a global screen point to content coordinate (0..1). */
Vec2f screen_to_content(double sx, double sy, const Monitor &m);

/**
 * Cursor target in OUTPUT coordinates (0..1).
 *
 * Deliberately based on the whole monitor, never on the content area: with
 * wallpaper padding the captured screen is inset, but the user's pointer still
 * travels across the entire monitor. Clamping to the content rectangle made
 * the drawn cursor stop at the padding edge before the physical pointer did.
 */
Vec2f screen_to_cursor(double sx, double sy, const Monitor &m);

/** Converts a content coordinate (0..1) to a global screen point. */
Vec2f content_to_screen(double cu, double cv, const Monitor &m);

/** true if the global point belongs to the monitor. */
bool monitor_contains(const Monitor &m, double sx, double sy);

} // namespace topscreen::core
