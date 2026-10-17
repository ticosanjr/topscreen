#pragma once

#include "spring.hpp"
#include "types.hpp"

namespace topscreen::core {

struct CursorConfig {
	bool enabled = true;
	int style = static_cast<int>(CursorStyle::MacOs);
	double size = 1.4;     // size multiplier
	bool shadow = true;
	double smooth = 0.0;   // 0 = follows the mouse exactly
	double sway = 0.0;     // 0..1 -> tilt on the curve
	bool hide_idle = true; // hides when idle
	double idle_delay_ms = 1200.0;
	bool loop = false; // returns to the start when idle
	double loop_ms = 4000.0;
};

struct CursorRender {
	double x = 0.5;
	double y = 0.5;
	double opacity = 1.0;
	double rotation = 0.0; // rad
	double size = 1.0;
	bool visible = true;
};

/**
 * Cursor model: smoothing, sway, idle and loop.
 *
 * Works in content coordinates (0..1). Draws nothing; it only produces
 * the state that the compositor uses to render.
 */
class CursorModel {
public:
	void configure(const CursorConfig &cfg);

	/** Sets the starting position (and the loop's return point). */
	void reset(double x, double y, double now_ms);

	/** Sets the target position (real pointer position). */
	void set_target(double x, double y, double now_ms);

	/** Advances the simulation up to now_ms. */
	void update(double now_ms);

	CursorRender render() const;
	bool visible() const { return render().visible; }

private:
	double stiffness() const;

	CursorConfig cfg_;
	Spring sx_;
	Spring sy_;
	double last_move_ms_ = 0.0;
	double last_update_ms_ = 0.0;
	double start_x_ = 0.5;
	double start_y_ = 0.5;
	double rotation_ = 0.0;
	bool started_ = false;
};

} // namespace topscreen::core
