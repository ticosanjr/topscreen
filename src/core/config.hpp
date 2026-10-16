#pragma once

#include "click_fx.hpp"
#include "cursor.hpp"
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
	double padding = 64.0;
	double corner_radius = 18.0;
	bool shadow = true;
	double shadow_alpha = 0.35;
	double shadow_blur = 48.0;
};

struct ClickSoundConfig {
	bool enabled = true;
	std::string file = "click-soft.wav";
	double volume = 0.6;
	bool left = true;
	bool right = false;
};

/** Configuracao completa do TopScreen (independente do OBS). */
struct Config {
	bool effects_enabled = true;
	BackgroundConfig bg;
	CursorConfig cursor;
	ZoomConfig zoom;
	ClickConfig click_left;
	ClickConfig click_right;
	ClickSoundConfig click_sound;
};

} // namespace topscreen::core
