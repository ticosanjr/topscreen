#pragma once

#include "core/config.hpp"

namespace topscreen::core {

/** Per-frame magnifier state consumed by the shader. */
struct MagnifierRender {
	double amount = 0.0; // 0..1 animated activation
	double zoom = 1.0;   // effective inside-lens zoom
	double focus_x = 0.5;
	double focus_y = 0.5;
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
	void update(double now_ms, double cursor_x, double cursor_y);
	MagnifierRender render() const;

	bool active() const { return target_; }
	bool visible() const { return amount_ > 0.001; }

private:
	void begin_transition(bool active, double now_ms);
	void update_focus(double cursor_x, double cursor_y);

	MagnifierConfig cfg_;
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
};

} // namespace topscreen::core
