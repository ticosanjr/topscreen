#include "ts-settings.hpp"

namespace topscreen {

void config_defaults(obs_data_t *s)
{
	obs_data_set_default_bool(s, key::kEffectsEnabled, true);

	obs_data_set_default_bool(s, key::kBgEnabled, true);
	obs_data_set_default_int(s, key::kBgMode, static_cast<int>(BgMode::Diagonal));
	obs_data_set_default_int(s, key::kBgColorA, 0xFF20242E);
	obs_data_set_default_int(s, key::kBgColorB, 0xFF3A2E5C);
	obs_data_set_default_double(s, key::kBgPadding, 64.0);
	obs_data_set_default_double(s, key::kBgCornerRadius, 18.0);
	obs_data_set_default_bool(s, key::kBgShadow, true);
	obs_data_set_default_double(s, key::kBgShadowAlpha, 0.35);
	obs_data_set_default_double(s, key::kBgShadowBlur, 48.0);

	obs_data_set_default_double(s, key::kZoom, 1.0);
	obs_data_set_default_double(s, key::kZoomFocusX, 0.5);
	obs_data_set_default_double(s, key::kZoomFocusY, 0.5);
}

void config_load(const obs_data_t *s, Config *out)
{
	out->effects_enabled = obs_data_get_bool(s, key::kEffectsEnabled);

	out->bg.enabled = obs_data_get_bool(s, key::kBgEnabled);
	out->bg.mode = static_cast<int>(obs_data_get_int(s, key::kBgMode));
	out->bg.color_a = obs_data_get_int(s, key::kBgColorA);
	out->bg.color_b = obs_data_get_int(s, key::kBgColorB);
	out->bg.padding = obs_data_get_double(s, key::kBgPadding);
	out->bg.corner_radius = obs_data_get_double(s, key::kBgCornerRadius);
	out->bg.shadow = obs_data_get_bool(s, key::kBgShadow);
	out->bg.shadow_alpha = obs_data_get_double(s, key::kBgShadowAlpha);
	out->bg.shadow_blur = obs_data_get_double(s, key::kBgShadowBlur);

	out->zoom.zoom = obs_data_get_double(s, key::kZoom);
	out->zoom.focus_x = obs_data_get_double(s, key::kZoomFocusX);
	out->zoom.focus_y = obs_data_get_double(s, key::kZoomFocusY);
}

} // namespace topscreen
