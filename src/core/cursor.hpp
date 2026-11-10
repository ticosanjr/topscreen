#pragma once

#include "types.hpp"

#include <cstddef>
#include <string_view>

namespace topscreen::core {

enum class CursorFollowMode : int {
	Exact = 0,   // follows the pointer immediately
	GlideAB = 3, // eases from the current point to the next accepted point
	Smooth = 4,  // continuous low-latency smoothing
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
	/* Extra rotation added to the style baseline (0° bundled, 30° Custom). */
	double rotation_offset_deg = 0.0;
	bool shadow = true;
	int follow_mode = static_cast<int>(CursorFollowMode::Exact);
	double glide_ms = 180.0; // Glide: A->B easing duration
	double smooth_ms = 90.0; // Smooth: low-pass time constant
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
 * Cursor model: exact/glide tracking and idle fade.
 *
 * Works in target/frame coordinates (0..1). Draws nothing; it only produces
 * the state that the compositor uses to render.
 */
class CursorModel {
public:
	void configure(const CursorConfig &cfg);

	/** Sets the starting position. */
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

	void start_glide(double now_ms);
	void step_glide(double now_ms);

	CursorConfig cfg_;
	double x_ = 0.5;
	double y_ = 0.5;
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
	bool glide_active_ = false;
	bool target_dirty_ = false;
	bool link_ = false;
	bool started_ = false;
	int resolved_style_ = 0;
	int resolved_link_style_ = -1;
	double style_rotation_ = 0.0;
	double link_rotation_ = 0.0;
};

} // namespace topscreen::core
