#include "ts-properties.hpp"

#include "core/easing.hpp"
#include "core/keys.hpp"
#include "core/types.hpp"

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen {

namespace {

void add_click_group(obs_properties_t *props, const char *group_key, const char *title_key,
		     const char *effect_key, const char *size_key, const char *color_key,
		     const char *duration_key, const char *easing_key)
{
	obs_properties_t *g = obs_properties_create();

	obs_property_t *effect = obs_properties_add_list(g, effect_key, obs_module_text("Click.Effect"),
							 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Ripple"),
				  static_cast<int>(ClickEffect::Ripple));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Orb"),
				  static_cast<int>(ClickEffect::Orb));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Ring"),
				  static_cast<int>(ClickEffect::Ring));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Force"),
				  static_cast<int>(ClickEffect::Force));

	obs_properties_add_float_slider(g, size_key, obs_module_text("Click.Size"), 0.1, 4.0, 0.05);
	obs_properties_add_color(g, color_key, obs_module_text("Click.Color"));
	obs_properties_add_float_slider(g, duration_key, obs_module_text("Click.DurationMs"), 100.0, 2000.0, 10.0);

	obs_property_t *easing = obs_properties_add_list(g, easing_key, obs_module_text("Click.Easing"),
							 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(easing, obs_module_text("Easing.Linear"), EASE_LINEAR);
	obs_property_list_add_int(easing, obs_module_text("Easing.InCubic"), EASE_IN_CUBIC);
	obs_property_list_add_int(easing, obs_module_text("Easing.OutCubic"), EASE_OUT_CUBIC);
	obs_property_list_add_int(easing, obs_module_text("Easing.InOutCubic"), EASE_IN_OUT_CUBIC);

	obs_properties_add_group(props, group_key, obs_module_text(title_key), OBS_GROUP_NORMAL, g);
}

} // namespace

obs_properties_t *properties_create(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_bool(props, kEffectsEnabled, obs_module_text("Enabled"));

	/* ---------------- Fundo ---------------- */
	obs_properties_t *bg = obs_properties_create();
	obs_properties_add_bool(bg, kBgEnabled, obs_module_text("Background.Enabled"));

	obs_property_t *mode = obs_properties_add_list(bg, kBgMode, obs_module_text("Background.Mode"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Solid"), static_cast<int>(BgMode::Solid));
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Vertical"),
				  static_cast<int>(BgMode::Vertical));
	obs_property_list_add_int(mode, obs_module_text("Background.Mode.Diagonal"),
				  static_cast<int>(BgMode::Diagonal));

	obs_properties_add_color(bg, kBgColorA, obs_module_text("Background.ColorA"));
	obs_properties_add_color(bg, kBgColorB, obs_module_text("Background.ColorB"));
	obs_properties_add_float_slider(bg, kBgPadding, obs_module_text("Background.Padding"), 0.0, 400.0, 1.0);
	obs_properties_add_float_slider(bg, kBgCornerRadius, obs_module_text("Background.CornerRadius"), 0.0, 200.0,
					1.0);
	obs_properties_add_bool(bg, kBgShadow, obs_module_text("Background.Shadow"));
	obs_properties_add_float_slider(bg, kBgShadowAlpha, obs_module_text("Background.ShadowAlpha"), 0.0, 1.0,
					0.01);
	obs_properties_add_float_slider(bg, kBgShadowBlur, obs_module_text("Background.ShadowBlur"), 0.0, 200.0,
					1.0);
	obs_properties_add_group(props, "grp_bg", obs_module_text("Group.Background"), OBS_GROUP_NORMAL, bg);

	/* ---------------- Cursor ---------------- */
	obs_properties_t *cur = obs_properties_create();
	obs_properties_add_bool(cur, kCursorEnabled, obs_module_text("Cursor.Enabled"));

	obs_property_t *style = obs_properties_add_list(cur, kCursorStyle, obs_module_text("Cursor.Style"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.MacOs"),
				  static_cast<int>(CursorStyle::MacOs));
	obs_property_list_add_int(style, obs_module_text("Cursor.Style.Touch"),
				  static_cast<int>(CursorStyle::Touch));

	obs_properties_add_float_slider(cur, kCursorSize, obs_module_text("Cursor.Size"), 0.5, 4.0, 0.05);
	obs_properties_add_float_slider(cur, kCursorSmooth, obs_module_text("Cursor.Smooth"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(cur, kCursorSway, obs_module_text("Cursor.Sway"), 0.0, 1.0, 0.01);
	obs_properties_add_bool(cur, kCursorShadow, obs_module_text("Cursor.Shadow"));
	obs_properties_add_bool(cur, kCursorHideIdle, obs_module_text("Cursor.HideIdle"));
	obs_properties_add_float_slider(cur, kCursorIdleDelayMs, obs_module_text("Cursor.IdleDelayMs"), 0.0,
					5000.0, 50.0);
	obs_properties_add_bool(cur, kCursorLoop, obs_module_text("Cursor.Loop"));
	obs_properties_add_float_slider(cur, kCursorLoopMs, obs_module_text("Cursor.LoopMs"), 1000.0, 20000.0,
					100.0);
	obs_properties_add_group(props, "grp_cursor", obs_module_text("Group.Cursor"), OBS_GROUP_NORMAL, cur);

	/* ---------------- Cliques ---------------- */
	add_click_group(props, "grp_click_left", "Group.ClickLeft", kClickLeftEffect, kClickLeftSize,
			kClickLeftColor, kClickLeftDurationMs, kClickLeftEasing);
	add_click_group(props, "grp_click_right", "Group.ClickRight", kClickRightEffect, kClickRightSize,
			kClickRightColor, kClickRightDurationMs, kClickRightEasing);

	/* ---------------- Som de clique ---------------- */
	obs_properties_t *snd = obs_properties_create();
	obs_properties_add_bool(snd, kClickSoundEnabled, obs_module_text("ClickSound.Enabled"));
	obs_properties_add_float_slider(snd, kClickSoundVolume, obs_module_text("ClickSound.Volume"), 0.0, 1.0,
					0.01);
	obs_properties_add_bool(snd, kClickSoundLeft, obs_module_text("ClickSound.Left"));
	obs_properties_add_bool(snd, kClickSoundRight, obs_module_text("ClickSound.Right"));
	obs_properties_add_group(props, "grp_sound", obs_module_text("Group.ClickSound"), OBS_GROUP_NORMAL, snd);

	/* ---------------- Zoom ---------------- */
	obs_properties_t *zoom = obs_properties_create();
	obs_property_t *zmode = obs_properties_add_list(zoom, kZoomMode, obs_module_text("Zoom.Mode"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Off"), static_cast<int>(ZoomMode::Off));
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Auto"), static_cast<int>(ZoomMode::Auto));
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Manual"), static_cast<int>(ZoomMode::Manual));

	obs_properties_add_float_slider(zoom, kZoomAmount, obs_module_text("Zoom.Amount"), 1.0, 4.0, 0.05);
	obs_properties_add_float_slider(zoom, kZoomDurationMs, obs_module_text("Zoom.DurationMs"), 100.0, 2000.0,
					10.0);
	obs_properties_add_float_slider(zoom, kZoomIdleTimeoutMs, obs_module_text("Zoom.IdleTimeoutMs"), 200.0,
					10000.0, 50.0);
	obs_properties_add_float_slider(zoom, kZoomPanAmount, obs_module_text("Zoom.PanAmount"), 0.0, 1.0, 0.01);
	obs_properties_add_group(props, "grp_zoom", obs_module_text("Group.Zoom"), OBS_GROUP_NORMAL, zoom);

	return props;
}

} // namespace topscreen
