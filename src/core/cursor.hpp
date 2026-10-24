#pragma once

#include "spring.hpp"
#include "types.hpp"

#include <cstddef>
#include <string_view>

namespace topscreen::core {

enum class CursorFollowMode : int {
	Exact = 0,      // follows the pointer immediately
	Spring = 1,     // current critically damped spring
	GlideSampled = 2, // samples the pointer and eases between samples
	GlideAB = 3,    // eases from the current point to the next accepted point
};

/** X11 cursor names that mean "link/hand" (Windows uses IDC_HAND instead). */
inline bool cursor_name_is_link(std::string_view name)
{
	if (name == "hand" || name == "hand1" || name == "hand2" || name == "pointer")
		return true;
	for (size_t i = 0; i + 4 <= name.size(); ++i) {
		char a = name[i];
		char b = name[i + 1];
		char c = name[i + 2];
		char d = name[i + 3];
		if ((a | 0x20) == 'l' && (b | 0x20) == 'i' && (c | 0x20) == 'n' &&
		    (d | 0x20) == 'k')
			return true;
	}
	return false;
}

struct CursorConfig {
	bool enabled = true;
	int style = static_cast<int>(CursorStyle::MacOs);
	/* -1 = same as the primary cursor. Used while the native pointer is a link. */
	int secondary_style = -1;
	bool link_detection = true;
	double size = 1.4;     // size multiplier
	bool shadow = true;
	int follow_mode = static_cast<int>(CursorFollowMode::Exact);
	double glide_ms = 180.0; // Glide: A->B easing duration
	/* Deprecated fields kept for profile compatibility; no UI. */
	double smooth = 0.0;
	double glide_sample_ms = 90.0;
	double sway = 0.0;
	bool loop = false;
	double loop_ms = 4000.0;
	double offset_x = 0.0;
	double offset_y = 0.0;
	bool hide_idle = true; // hides when idle
	double idle_delay_ms = 1200.0;
};

struct CursorRender {
	double x = 0.5;
	double y = 0.5;
	double opacity = 1.0;
	double rotation = 0.0; // rad
	double size = 1.0;
	int style = -1; // resolved atlas style; -1 lets the caller use the config
	bool visible = true;
};

/**
 * Cursor model: smoothing, sway, idle and loop.
 *
 * Works in target/frame coordinates (0..1). Draws nothing; it only produces
 * the state that the compositor uses to render.
 */
class CursorModel {
public:
	void configure(const CursorConfig &cfg);

	/** Sets the starting position (and the loop's return point). */
	void reset(double x, double y, double now_ms);

	/** Sets the target position (real pointer position). */
	void set_target(double x, double y, double now_ms);

	/** Native pointer is over a link (Windows/X11 only; false elsewhere). */
	void set_link(bool link) { link_ = link; }

	/** Advances the simulation up to now_ms. */
	void update(double now_ms);

	CursorRender render() const;
	bool visible() const { return render().visible; }

private:
	double stiffness() const;

	void start_glide(double now_ms);
	void step_glide(double now_ms);

	CursorConfig cfg_;
	Spring sx_;
	Spring sy_;
	double last_move_ms_ = 0.0;
	double last_update_ms_ = 0.0;
	double start_x_ = 0.5;
	double start_y_ = 0.5;
	double pointer_x_ = 0.5;
	double pointer_y_ = 0.5;
	double target_x_ = 0.5;
	double target_y_ = 0.5;
	double glide_from_x_ = 0.5;
	double glide_from_y_ = 0.5;
	double glide_to_x_ = 0.5;
	double glide_to_y_ = 0.5;
	double glide_ctrl_x_ = 0.5;
	double glide_ctrl_y_ = 0.5;
	double glide_start_ms_ = 0.0;
	double glide_end_ms_ = 0.0;
	double next_sample_ms_ = 0.0;
	bool glide_active_ = false;
	bool target_dirty_ = false;
	bool link_ = false;
	double rotation_ = 0.0;
	bool started_ = false;
};

} // namespace topscreen::core
