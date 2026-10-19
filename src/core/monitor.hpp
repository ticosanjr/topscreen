#pragma once

#include "coord.hpp"

#include <cstdint>

namespace topscreen::core {

/**
 * Picks the monitor a capture belongs to.
 *
 * Monitor lists differ per platform, and a capture only reports its size, so
 * the choice is: a hint index with a matching size, then a unique size match,
 * then (only when trusted) the hint index itself. Returns false when nothing
 * can be decided, in which case callers keep the origin at (0,0).
 *
 * `trust_hint` must be false whenever the hint does not come from a real
 * monitor index (e.g. X11's "screen" is an X screen, not a monitor).
 */
bool pick_monitor(const Monitor *monitors, int count, int hint, bool trust_hint,
		  uint32_t capture_w, uint32_t capture_h, Monitor *out);

} // namespace topscreen::core
