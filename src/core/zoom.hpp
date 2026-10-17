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
