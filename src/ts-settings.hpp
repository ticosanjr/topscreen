#pragma once

#include <obs-module.h>

#include <cstdint>

/**
 * Modelo de configuracao do TopScreen (persistido pelo OBS via obs_data).
 *
 * Cores usam o formato do OBS: inteiro 0xAABBGGRR, consistente com
 * obs_properties_add_color() e gs_effect_set_color().
 */
namespace topscreen {

namespace key {
inline constexpr const char *kEffectsEnabled = "effects_enabled";

inline constexpr const char *kBgEnabled = "bg_enabled";
inline constexpr const char *kBgMode = "bg_mode";
inline constexpr const char *kBgColorA = "bg_color_a";
inline constexpr const char *kBgColorB = "bg_color_b";
inline constexpr const char *kBgPadding = "bg_padding";
inline constexpr const char *kBgCornerRadius = "bg_corner_radius";
inline constexpr const char *kBgShadow = "bg_shadow";
inline constexpr const char *kBgShadowAlpha = "bg_shadow_alpha";
inline constexpr const char *kBgShadowBlur = "bg_shadow_blur";

inline constexpr const char *kZoom = "zoom";
inline constexpr const char *kZoomFocusX = "zoom_focus_x";
inline constexpr const char *kZoomFocusY = "zoom_focus_y";
} // namespace key

enum class BgMode : int {
	Solid = 0,
	Vertical = 1,
	Diagonal = 2,
};

struct BackgroundCfg {
	bool enabled = true;
	int mode = static_cast<int>(BgMode::Diagonal);
	long long color_a = 0xFF20242E; // 0xAABBGGRR
	long long color_b = 0xFF3A2E5C; // 0xAABBGGRR
	double padding = 64.0;
	double corner_radius = 18.0;
	bool shadow = true;
	double shadow_alpha = 0.35;
	double shadow_blur = 48.0;
};

struct ZoomCfg {
	double zoom = 1.0;
	double focus_x = 0.5;
	double focus_y = 0.5;
};

struct Config {
	bool effects_enabled = true;
	BackgroundCfg bg;
	ZoomCfg zoom;
};

/** Preenche os defaults no obs_data (usado em get_defaults). */
void config_defaults(obs_data_t *settings);

/** Le a configuracao a partir do obs_data. */
void config_load(const obs_data_t *settings, Config *out);

} // namespace topscreen
