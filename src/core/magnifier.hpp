#pragma once

#include "core/config.hpp"

namespace topscreen::core {

/** Per-frame magnifier state consumed by the shader. */
struct MagnifierRender {
	double amount = 0.0; // 0..1 animated activation
	double zoom = 1.0;   // effective inside-lens zoom
	/* Canvas center (output 0..1). Clamped near the edges so the lens stays
	 * inside the screen. */
	double focus_x = 0.5;
	double focus_y = 0.5;
	/* Magnification anchor (output 0..1): the content under this point stays
	 * under the pointer. Follows the cursor all the way to the screen edges,
	 * even when the canvas is clamped. */
	double sample_x = 0.5;
	double sample_y = 0.5;
	double drop = 0.0; // vertical offset in px (negative while dropping in)
};

/**
 * Animates the magnifier lens toggle. The lens follows the real pointer when
 * configured, or stays at a fixed content-space center. Spotlight and the
 * magnifier are mutually exclusive (see sanitize_config / the UI callbacks).
 */
class MagnifierModel {
public:
	void configure(const MagnifierConfig &cfg);
	void set_active(bool active, double now_ms);
	/** `aspect` is target width / target height, used to keep the round lens
	 * inside the screen when x and y have different pixel scales. */
	void update(double now_ms, double cursor_x, double cursor_y, double aspect = 1.0);
	MagnifierRender render() const;

	bool active() const { return target_; }
	bool visible() const { return amount_ > 0.001; }

private:
	void begin_transition(bool active, double now_ms);
	void update_focus(double cursor_x, double cursor_y, double aspect);

	MagnifierConfig cfg_;
	bool initialized_ = false;
	bool target_ = false;
	double amount_ = 0.0;
	double from_ = 0.0;
	double to_ = 0.0;
	double start_ms_ = 0.0;
	double duration_ms_ = 0.0;
	double last_ms_ = 0.0;
	double focus_x_ = 0.5;  // canvas center
	double focus_y_ = 0.5;
	double sample_x_ = 0.5; // magnification anchor (pointer)
	double sample_y_ = 0.5;
};

} // namespace topscreen::core
