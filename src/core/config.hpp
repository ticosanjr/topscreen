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
	int mode = static_cast<int>(BgMode::Diagonal);
	uint32_t color_a = 0xFF20242E; // 0xAABBGGRR
	uint32_t color_b = 0xFF3A2E5C; // 0xAABBGGRR
	std::string image;             // wallpaper path/relative ("" = none)
	std::string image_file;        // PC image (takes priority over `image`)
	double image_blur = 0.0;       // wallpaper blur (px)
	int image_blur_type = 2;       // 0 none, 1 soft, 2 gaussian, 3 creamy
	double padding_x = 64.0;
	double padding_y = 64.0;
	double corner_radius = 18.0;
	bool shadow = true;
	double shadow_alpha = 0.35;
	double shadow_blur = 48.0;
	double shadow_offset = 12.0; // vertical offset (px)
	bool keep_aspect = true;     // keeps the screen aspect ratio (without distortion)
};

struct ClickSoundConfig {
	bool enabled = true;
	std::string file = "sounds/click-soft.wav";
	double volume = 0.6;
	bool left = true;
	bool right = false;
};

/** Motion blur (radial, tied to the zoom/cursor movement). */
struct MotionBlurConfig {
	bool enabled = true;
	double amount = 0.35; // 0..1
};

/** Spotlight: darkens outside a circle that follows the cursor. */
struct SpotlightConfig {
	bool enabled = false;
	double radius = 0.30; // fraction of the shorter side
	double dim = 0.60;    // 0..1 (how much it darkens outside)
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
	KeyOverlayConfig keys;
	HighlighterConfig highlight;
};

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

} // namespace topscreen::core
