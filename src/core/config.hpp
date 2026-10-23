#pragma once

#include "click_fx.hpp"
#include "cursor.hpp"
#include "highlighter.hpp"
#include "key_overlay.hpp"
#include "types.hpp"
#include "zoom.hpp"

#include <cstdint>
#include <string>

namespace topscreen::core {

struct BackgroundConfig {
	bool enabled = true;
	int mode = static_cast<int>(BgMode::Gradient); // 0 solid, 1 gradient, 2 wallpaper
	uint32_t color_a = 0xFF20242E; // 0xAABBGGRR
	uint32_t color_b = 0xFF3A2E5C; // 0xAABBGGRR
	double gradient_angle = 45.0; // degrees; 0 = left→right
	double gradient_center_x = 0.5;
	double gradient_center_y = 0.5;
	std::string image;             // wallpaper path/relative ("" = none)
	std::string image_file;        // PC image (takes priority over `image`)
	double image_blur = 0.0;       // wallpaper blur (px)
	int image_blur_type = 2;       // 0 none, 1 soft, 2 gaussian, 3 creamy, 4 frosted
	double padding_x = 64.0;
	double padding_y = 64.0;
	double corner_radius = 18.0;
	bool shadow = true;
	double shadow_alpha = 0.35;
	double shadow_blur = 48.0;
	double shadow_offset_x = 0.0; // horizontal offset (px)
	double shadow_offset = 12.0;  // vertical offset (px)
	bool keep_aspect = true;      // keeps the screen aspect ratio (without distortion)
};

struct ClickSoundConfig {
	bool enabled = true;
	std::string file_left = "sounds/click-soft.wav";
	std::string file_right = "sounds/click-mouse-right.wav";
	double volume = 0.6;
	bool left = true;
	bool right = false;
};

/** Motion blur (radial, tied to the zoom/cursor movement). */
struct MotionBlurConfig {
	bool enabled = true;
	double amount = 0.35; // 0..1
};

/** Spotlight: darkens outside a shape that follows the cursor. */
enum class SpotlightShape : int {
	Circle = 0,
	Square = 1,
};

struct SpotlightConfig {
	bool enabled = false;
	int shape = static_cast<int>(SpotlightShape::Circle);
	double radius = 0.30; // fraction of the shorter side
	double corner_radius = 0.0; // square only, px
	double dim = 0.60;          // 0..1 (how much it darkens outside)

	bool border_enabled = false;
	uint32_t border_color = 0xFFFFFFFFu;
	double border_width = 4.0; // px

	bool shadow_enabled = false;
	uint32_t shadow_color = 0xFF000000u;
	double shadow_alpha = 0.55;
	double shadow_blur = 30.0; // px
	double shadow_offset_x = 0.0;
	double shadow_offset_y = 12.0;

	bool blur_enabled = false;
	double blur_amount = 16.0; // px

	double zoom_amount = 1.35; // multiplicative over the current zoom
	double in_ms = 250.0;
	double out_ms = 300.0;
	double drop_px = 24.0;
	bool show_cursor = true;
};

/** Magnifier lens: zooms only inside a circle/square, independent of Spotlight. */
struct MagnifierConfig {
	bool enabled = false;
	int shape = static_cast<int>(SpotlightShape::Circle);
	double radius = 0.28;       // fraction of the shorter side
	double corner_radius = 0.0; // square only, px
	double zoom_amount = 2.0;   // inside-lens magnification
	bool follow_cursor = true;
	double center_x = 0.5; // fixed center when follow_cursor is off
	double center_y = 0.5;

	bool border_enabled = false;
	uint32_t border_color = 0xFFFFFFFFu;
	double border_width = 4.0;

	bool shadow_enabled = false;
	uint32_t shadow_color = 0xFF000000u;
	double shadow_alpha = 0.55;
	double shadow_blur = 30.0;
	double shadow_offset_x = 0.0;
	double shadow_offset_y = 12.0;

	double in_ms = 250.0;
	double out_ms = 300.0;
	double drop_px = 24.0;
	bool show_cursor = true;
};

/** Complete TopScreen configuration (independent of OBS). */
struct Config {	bool effects_enabled = true;
	BackgroundConfig bg;
	CursorConfig cursor;
	ZoomConfig zoom;
	ClickConfig click_left;
	ClickConfig click_right;
	ClickSoundConfig click_sound;
	MotionBlurConfig motion_blur;
	SpotlightConfig spotlight;
	MagnifierConfig magnifier;
	KeyOverlayConfig keys;
	HighlighterConfig highlight;

	/* Origin of the captured monitor in global desktop coordinates.
	 * Automatic by default; `manual` wins when the platform detection cannot
	 * decide (e.g. identical monitors stacked vertically). */
	bool monitor_manual = false;
	int monitor_x = 0;
	int monitor_y = 0;
};

/**
 * Bounds every numeric field and replaces non-finite doubles with the default.
 *
 * Imported presets/profiles and older settings can contain values outside the
 * ranges exposed by the UI (or even NaN). The render and animation models
 * already clamp many values locally, but doing it once here keeps imported
 * data from producing degenerate uniforms or states. Valid values are kept
 * unchanged; only out-of-range input is clamped.
 */
void sanitize_config(Config *cfg);

/**
 * Chooses the effective wallpaper path.
 *
 * Rule: the "PC Image" wins if filled in; otherwise the built-in wallpaper.
 * (The interface keeps the two mutually exclusive.)
 */
inline const char *resolve_bg_path(const char *wallpaper, const char *image_file)
{
	if (image_file && *image_file)
		return image_file;
	if (wallpaper && *wallpaper)
		return wallpaper;
	return "";
}

/**
 * Legacy background migration.
 *
 * Old sources and presets stored `bg_mode` as 0 solid, 1 vertical gradient,
 * 2 diagonal gradient and 3 wallpaper. The current `bg_type` is 0 solid,
 * 1 gradient and 2 wallpaper. Keep both conversions here so settings and
 * presets cannot drift apart.
 */
inline int legacy_bg_mode_value(int mode)
{
	return mode == static_cast<int>(BgMode::Wallpaper) ? 3 : mode;
}

inline int bg_mode_from_legacy(int legacy_mode, bool has_wallpaper)
{
	if (has_wallpaper || legacy_mode == 3)
		return static_cast<int>(BgMode::Wallpaper);
	if (legacy_mode == 0)
		return static_cast<int>(BgMode::Solid);
	return static_cast<int>(BgMode::Gradient);
}

inline double bg_gradient_angle_from_legacy(int legacy_mode, double fallback)
{
	if (legacy_mode == 1)
		return 90.0;
	if (legacy_mode == 2)
		return 45.0;
	return fallback;
}

} // namespace topscreen::core
