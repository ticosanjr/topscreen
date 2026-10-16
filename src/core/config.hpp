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
	std::string image;             // caminho/relativo do wallpaper ("" = nenhum)
	std::string image_file;        // imagem do PC (tem prioridade sobre `image`)
	double image_blur = 0.0;       // desfoque do wallpaper (px)
	double padding_x = 64.0;
	double padding_y = 64.0;
	double corner_radius = 18.0;
	bool shadow = true;
	double shadow_alpha = 0.35;
	double shadow_blur = 48.0;
	double shadow_offset = 12.0; // deslocamento vertical (px)
	bool keep_aspect = true;     // mantem a proporcao da tela (sem distorcao)
};

struct ClickSoundConfig {
	bool enabled = true;
	std::string file = "sounds/click-soft.wav";
	double volume = 0.6;
	bool left = true;
	bool right = false;
};

/** Motion blur (radial, ligado ao movimento do zoom/cursor). */
struct MotionBlurConfig {
	bool enabled = true;
	double amount = 0.35; // 0..1
};

/** Spotlight: escurece fora de um circulo que segue o cursor. */
struct SpotlightConfig {
	bool enabled = false;
	double radius = 0.30; // fracao do menor lado
	double dim = 0.60;    // 0..1 (quanto escurece fora)
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
	MotionBlurConfig motion_blur;
	SpotlightConfig spotlight;
	KeyOverlayConfig keys;
	HighlighterConfig highlight;
};

} // namespace topscreen::core
