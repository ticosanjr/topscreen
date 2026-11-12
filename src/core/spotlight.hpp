#pragma once

#include "core/config.hpp"

namespace topscreen::core {

/** Per-frame spotlight state consumed by the shader. */
struct SpotlightRender {
	double amount = 0.0; // 0..1 animated activation
	double zoom = 1.0;   // multiplicative zoom factor
	double focus_x = 0.5; // displayed mask center
	double focus_y = 0.5;
	double zoom_focus_x = 0.5; // source point under the cursor
	double zoom_focus_y = 0.5;
};

/**
 * Animates the spotlight toggle: fade/zoom in on activation and the reverse
 * on deactivation. The mask itself follows the cursor and is rendered
 * by the shader.
 */
class SpotlightModel {
public:
	void configure(const SpotlightConfig &cfg);
	void set_active(bool active, double now_ms);
	void update(double now_ms, double cursor_x, double cursor_y, double zoom_focus_x,
		    double zoom_focus_y);
	/** Test/legacy convenience: zoom focus equals the displayed cursor. */
	void update(double now_ms, double cursor_x, double cursor_y)
	{
		update(now_ms, cursor_x, cursor_y, cursor_x, cursor_y);
	}
	SpotlightRender render() const;

	bool active() const { return target_; }
	bool visible() const { return amount_ > 0.001; }

private:
	void begin_transition(bool active, double now_ms);

	SpotlightConfig cfg_;
	bool initialized_ = false;
	bool target_ = false;
	double amount_ = 0.0;
	double from_ = 0.0;
	double to_ = 0.0;
	double start_ms_ = 0.0;
	double duration_ms_ = 0.0;
	double last_ms_ = 0.0;
	double cursor_x_ = 0.5;
	double cursor_y_ = 0.5;
	double zoom_focus_x_ = 0.5;
	double zoom_focus_y_ = 0.5;
};

} // namespace topscreen::core
