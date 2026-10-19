#pragma once

#include "easing.hpp"
#include "spring.hpp"
#include "types.hpp"

namespace topscreen::core {

struct ZoomConfig {
	int mode = static_cast<int>(ZoomMode::Auto);
	double amount = 1.6;         // target zoom in auto mode
	double duration_ms = 450.0;  // animation speed
	double idle_timeout_ms = 1500.0; // no focus -> returns to normal
	double pan_amount = 1.0;     // how much the focus "follows" the cursor (0..1)
	double center_amount = 1.0;  // how much the cursor stays at the CENTER (0..1)
	int easing = EASE_IN_OUT_CUBIC;
};

struct ZoomRender {
	double zoom = 1.0;
	double focus_x = 0.5;
	double focus_y = 0.5;
};

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

	/**
	 * Zoom-spotlight gesture: quick zoom into a point, short hold, slow return.
	 * Takes precedence over the auto/manual zoom while active.
	 */
	void spot_start(double cx, double cy, double target_zoom, double in_ms, double hold_ms,
			double out_ms, double now_ms);
	void spot_stop();
	bool spot_active() const { return spot_active_; }

	/** Manual step: factor > 1 zooms in, factor < 1 zooms out. */
	void manual_step(double factor);
	void set_manual(double value);
	double manual() const { return manual_; }

	void update(double now_ms);

	ZoomRender render() const;

	/** Current zoom velocity (for motion blur). */
	double zoom_velocity() const { return zoom_.velocity(); }

private:
	double stiffness() const;

	ZoomConfig cfg_;
	bool spot_active_ = false;
	double spot_cx_ = 0.5;
	double spot_cy_ = 0.5;
	double spot_target_ = 1.0;
	double spot_start_ms_ = 0.0;
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
	double last_update_ms_ = 0.0;
	bool started_ = false;
};

} // namespace topscreen::core
