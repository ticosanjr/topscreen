#pragma once

#include "core/coord.hpp"
#include "core/monitor.hpp"

#include <cstdint>

namespace topscreen {

/**
 * Monitor geometry for the input mapping.
 *
 * The cursor comes in GLOBAL desktop coordinates, while a capture covers only
 * one monitor. Without the monitor origin, a monitor placed below/left of the
 * primary one puts every effect at the wrong place (the classic "cursor is
 * offset vertically when the capture is the lower screen").
 */

/** Fills up to `max` monitor rectangles in global coordinates (platform order). */
int ts_query_monitors(core::Monitor *out, int max);

/**
 * Origin of the monitor for a capture of `capture_w` x `capture_h`.
 *
 * `device_id` (optional) is the platform device name the capture points at
 * (`\\.\DISPLAY2` on Windows): the most precise source of truth.
 * `hint` is the capture's monitor index, used only when `trust_hint`.
 * Returns false when it cannot be decided (caller keeps 0,0).
 */
bool ts_resolve_monitor_origin(const char *device_id, int hint, bool trust_hint, uint32_t capture_w,
			       uint32_t capture_h, double *origin_x, double *origin_y);

/** Full rectangle of the monitor a capture belongs to (same detection rules). */
bool ts_resolve_monitor(const char *device_id, int hint, bool trust_hint, uint32_t capture_w,
			uint32_t capture_h, core::Monitor *out);

/**
 * Manual override from the environment: TOPSCREEN_MONITOR_ORIGIN="x,y".
 * Useful for exotic layouts and for tests.
 */
bool ts_monitor_origin_override(double *origin_x, double *origin_y);

/** Monitor index whose device name matches (Windows); -1 when unknown. */
int ts_monitor_index_for_device(const char *device);

} // namespace topscreen
