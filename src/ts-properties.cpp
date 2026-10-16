#include "ts-properties.hpp"

#include "ts-settings.hpp"

namespace topscreen {

obs_properties_t *properties_create(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_bool(props, key::kEffectsEnabled, obs_module_text("Enabled"));

	/* ---------------- Fundo ---------------- */
	obs_properties_t *bg = obs_properties_create();

	obs_properties_add_bool(bg, key::kBgEnabled, obs_module_text("Background.Enabled"));

	obs_property_t *mode = obs_properties_add_list(bg, key::kBgMode, obs_module_text("Background.Mode"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Solid"), static_cast<int>(BgMode::Solid));
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Vertical"),
				  static_cast<int>(BgMode::Vertical));
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Diagonal"),
				  static_cast<int>(BgMode::Diagonal));

	obs_properties_add_color(bg, key::kBgColorA, obs_module_text("Background.ColorA"));
	obs_properties_add_color(bg, key::kBgColorB, obs_module_text("Background.ColorB"));

	obs_properties_add_float_slider(bg, key::kBgPadding, obs_module_text("Background.Padding"), 0.0, 400.0,
					1.0);
	obs_properties_add_float_slider(bg, key::kBgCornerRadius, obs_module_text("Background.CornerRadius"), 0.0,
					200.0, 1.0);

	obs_properties_add_bool(bg, key::kBgShadow, obs_module_text("Background.Shadow"));
	obs_properties_add_float_slider(bg, key::kBgShadowAlpha, obs_module_text("Background.ShadowAlpha"), 0.0,
					1.0, 0.01);
	obs_properties_add_float_slider(bg, key::kBgShadowBlur, obs_module_text("Background.ShadowBlur"), 0.0,
					200.0, 1.0);

	obs_properties_add_group(props, "grp_bg", obs_module_text("Group.Background"), OBS_GROUP_NORMAL, bg);

	/* ---------------- Zoom ---------------- */
	obs_properties_t *zoom = obs_properties_create();

	obs_properties_add_float_slider(zoom, key::kZoom, obs_module_text("Zoom.Amount"), 1.0, 4.0, 0.01);
	obs_properties_add_float_slider(zoom, key::kZoomFocusX, obs_module_text("Zoom.FocusX"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(zoom, key::kZoomFocusY, obs_module_text("Zoom.FocusY"), 0.0, 1.0, 0.01);

	obs_properties_add_group(props, "grp_zoom", obs_module_text("Group.Zoom"), OBS_GROUP_NORMAL, zoom);

	return props;
}

} // namespace topscreen
