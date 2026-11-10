#pragma once

#include "spring.hpp"
#include "types.hpp"

namespace topscreen::core {

struct ZoomConfig {
	int mode = static_cast<int>(ZoomMode::Auto);
	double amount = 1.6;         // target zoom in auto mode
	double duration_ms = 450.0;  // animation speed
	double idle_timeout_ms = 1500.0; // no focus -> returns to normal
	double smart_hold_ms = 1200.0;   // Smart: stay zoomed after the last action
	double pan_amount = 1.0;     // how much the focus "follows" the cursor (0..1)
	double center_amount = 1.0;  // how much the cursor stays at the CENTER (0..1)
	bool motion_blur = false;    // optional radial blur while zooming
	double motion_blur_amount = 0.35;
};

struct ZoomRender {
	double zoom = 1.0;
	double focus_x = 0.5;
	double focus_y = 0.5;
	/* Absolute zoom change per second; the compositor turns it into a
	 * short-lived radial motion blur. */
	double velocity = 0.0;
};

/**
 * Where a hold-to-zoom gesture should go, computed from the drag (or the lack
 * of one) and the zoom that is on screen right now.
 *
 * - With a real rectangle: zoom so the rectangle fills the view, centered on it.
 * - Without one (plain hold): zoom to `point_zoom`, centered on the cursor.
 * - `zoom_out`: widen from the base instead (never below 1x).
 * `valid` is false when the result would be a no-op (e.g. zooming out at 1x).
 */
struct SpotAim {
	bool valid = false;
	double zoom = 1.0;
	double focus_x = 0.5;
	double focus_y = 0.5;
};

SpotAim spot_aim_for_hold(bool has_rect, double rx0, double ry0, double rx1, double ry1,
			  double cursor_x, double cursor_y, double base_zoom, double point_zoom,
			  double max_zoom, bool zoom_out);

/**
 * Zoom/pan model.
 *
 * Combines manual zoom (step by step via hotkey) with automatic zoom
 * (focus by click/follow cursor). Uses springs for everything, avoiding jumps.
 * All focus is bounded by clamp_focus() so it never samples outside the screen.
 */
class ZoomModel {
public:
	void configure(const ZoomConfig &cfg);
	void reset(double now_ms);

	/** Focus by click (auto-zoom). */
	void focus_on(double x, double y, double now_ms);

	/** Focus following the cursor. */
	void set_cursor(double x, double y, double now_ms);
	/** Marks an action (click/drag/typing) for Smart mode. */
	void note_activity(double now_ms);

	/**
	 * Starts a hold-to-zoom gesture at an ABSOLUTE zoom target.
	 *
	 * Callers compute the target first (see core::spot_aim_for_* below); the
	 * model only animates base -> target while held and target -> base after
	 * release(). The base is whatever is on screen right now (which may
	 * already be zoomed) and the gesture returns to it.
	 * @param focus_x/focus_y content point kept fixed while zoomed.
	 */
	void spot_start_hold(double focus_x, double focus_y, double target, double in_ms,
			     double hold_ms, double out_ms, double now_ms);
	/** Releases a held spot: the configured hold runs, then it eases back. */
	void spot_release(double now_ms);
	void spot_stop();
	bool spot_active() const { return spot_active_; }
	bool spot_held() const { return spot_held_; }
	double spot_target() const { return spot_target_; }

	/** Manual step: factor > 1 zooms in, factor < 1 zooms out. */
	void manual_step(double factor);
	void set_manual(double value);
	double manual() const { return manual_; }

	void update(double now_ms);

	ZoomRender render() const;

private:
	double stiffness() const;

	ZoomConfig cfg_;
	bool spot_active_ = false;
	bool spot_held_ = false;
	double spot_cx_ = 0.5;
	double spot_cy_ = 0.5;
	double spot_target_ = 1.0;
	double spot_base_ = 1.0;
	double spot_base_fx_ = 0.5;
	double spot_base_fy_ = 0.5;
	double spot_release_zoom_ = 1.0;
	double spot_start_ms_ = 0.0;
	double spot_release_ms_ = 0.0;
	double spot_in_ms_ = 250.0;
	double spot_hold_ms_ = 600.0;
	double spot_out_ms_ = 1200.0;
	/** Zoom/focus of the current spot frame (valid while spot_active_). */
	double spot_zoom_at(double now_ms) const;
	double spot_focus_at(double now_ms) const;

	Spring zoom_;
	Spring fx_;
	Spring fy_;
	double manual_ = 1.0;
	double focus_point_x_ = 0.5; // last (source) point focused
	double focus_point_y_ = 0.5;
	double last_focus_ms_ = 0.0;
	double last_activity_ms_ = -1.0e9;
	double velocity_ = 0.0;
	double last_update_ms_ = 0.0;
	bool started_ = false;
};

} // namespace topscreen::core
