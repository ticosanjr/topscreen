#include "properties/ts-properties-internal.hpp"

#include "audio/ts-click-source.hpp"
#include "core/config_limits.hpp"
#include "core/cursor.hpp"
#include "core/cursor_catalog.hpp"
#include "core/keys.hpp"
#include "core/sound_catalog.hpp"
#include "core/types.hpp"

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen::properties {

bool cursor_options_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kCursorEnabled, true);
	const bool hide = setting_bool(settings, kCursorHideIdle, true);
	const long long mode =
		setting_int(settings, kCursorFollowMode,
			    static_cast<long long>(CursorFollowMode::Exact));
	const bool glide = mode == static_cast<long long>(CursorFollowMode::GlideAB);
	const bool smooth = mode == static_cast<long long>(CursorFollowMode::Smooth);
	if (!on) {
		for (const char *key : {kCursorStyle, kCursorSecondaryStyle,
				       kCursorLinkDetection, kCursorSize,
				       kCursorRotationOffset, kCursorFollowMode,
				       kCursorGlideMs, kCursorShadow, kCursorHideIdle,
				       kCursorIdleDelayMs}) {
			if (obs_property_t *v = obs_properties_get(props, key))
				obs_property_set_visible(v, false);
		}
		return true;
	}
	if (obs_property_t *v = obs_properties_get(props, kCursorStyle))
		obs_property_set_visible(v, true);
	if (obs_property_t *v = obs_properties_get(props, kCursorSecondaryStyle))
		obs_property_set_visible(v, true);
	if (obs_property_t *v = obs_properties_get(props, kCursorLinkDetection))
		obs_property_set_visible(v, true);
	if (obs_property_t *v = obs_properties_get(props, kCursorSize))
		obs_property_set_visible(v, true);
	if (obs_property_t *v = obs_properties_get(props, kCursorRotationOffset))
		obs_property_set_visible(v, true);
	if (obs_property_t *v = obs_properties_get(props, kCursorFollowMode))
		obs_property_set_visible(v, true);
	if (obs_property_t *v = obs_properties_get(props, kCursorShadow))
		obs_property_set_visible(v, true);
	if (obs_property_t *v = obs_properties_get(props, kCursorHideIdle))
		obs_property_set_visible(v, true);
	if (obs_property_t *v = obs_properties_get(props, kCursorGlideMs))
		obs_property_set_visible(v, glide);
	if (obs_property_t *v = obs_properties_get(props, kCursorSmoothMs))
		obs_property_set_visible(v, smooth);
	if (obs_property_t *v = obs_properties_get(props, kCursorIdleDelayMs))
		obs_property_set_visible(v, hide);
	return true;
}

void add_click_group(obs_properties_t *props, const char *group_key, const char *title,
		     const char *effect_key, const char *size_key, const char *color_key,
		     const char *duration_key)
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
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Glow"),
				  static_cast<int>(ClickEffect::Glow));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Echo"),
				  static_cast<int>(ClickEffect::Echo));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.Flare"),
				  static_cast<int>(ClickEffect::Flare));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.QuietCircle"),
				  static_cast<int>(ClickEffect::QuietCircle));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.WaterRipple"),
				  static_cast<int>(ClickEffect::WaterRipple));
	obs_property_list_add_int(effect, obs_module_text("Click.Effect.WaterRippleClear"),
				  static_cast<int>(ClickEffect::WaterRippleClear));
	set_tip(effect, obs_module_text("Tooltip.ClickEffect"));

	obs_property_t *size = add_range_slider(
		g, size_key, obs_module_text("Click.Size"),
		limits::kClickSizeRange);
	set_tip(size, obs_module_text("Tooltip.ClickSize"));
	obs_property_t *color =
		obs_properties_add_color(g, color_key, obs_module_text("Click.Color"));
	set_tip(color, obs_module_text("Tooltip.ClickColor"));
	obs_property_t *duration = add_range_slider(
		g, duration_key, obs_module_text("Click.DurationMs"),
		limits::kClickDurationRange);
	set_tip(duration, obs_module_text("Tooltip.ClickDuration"));

	obs_properties_add_group(props, group_key, title, OBS_GROUP_NORMAL, g);
}

bool click_left_effect_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool clear = settings &&
			   obs_data_get_int(settings, kClickLeftEffect) ==
				   static_cast<long long>(ClickEffect::WaterRippleClear);
	if (obs_property_t *c = obs_properties_get(props, kClickLeftColor))
		obs_property_set_visible(c, !clear);
	return true;
}

bool click_right_effect_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool clear = settings &&
			   obs_data_get_int(settings, kClickRightEffect) ==
				   static_cast<long long>(ClickEffect::WaterRippleClear);
	if (obs_property_t *c = obs_properties_get(props, kClickRightColor))
		obs_property_set_visible(c, !clear);
	return true;
}

bool click_sound_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = settings && obs_data_get_bool(settings, kClickSoundEnabled);
	for (const char *key : {kClickSoundFileLeft, kClickSoundFileRight, kClickSoundVolume}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	return true;
}

bool click_sound_side_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const char *left = settings ? obs_data_get_string(settings, kClickSoundFileLeft) : "";
	const char *right = settings ? obs_data_get_string(settings, kClickSoundFileRight) : "";
	obs_data_set_bool(settings, kClickSoundLeft, left && *left);
	obs_data_set_bool(settings, kClickSoundRight, right && *right);
	return click_sound_changed(props, nullptr, settings);
}

bool add_click_audio_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	(void)props;
	(void)p;
	(void)data;
	return ts_click_audio_add_to_current_scene();
}

bool add_click_audio_all_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	(void)props;
	(void)p;
	(void)data;
	return ts_click_audio_add_to_all_scenes();
}

void add_cursor_section(obs_properties_t *props, obs_properties_t *pointer, obs_data_t *settings)
{
	/* ---------------- Cursor ---------------- */
	obs_properties_t *cur = obs_properties_create();
	obs_property_t *style = obs_properties_add_list(cur, kCursorStyle, obs_module_text("Cursor.Style"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	/* One table drives the whole dropdown: same theme = adjacent entries and
	 * labels that name the theme first ("Bibata — Arrow"). */
	for (const auto &entry : kCursorCatalog)
		obs_property_list_add_int(style, obs_module_text(entry.label_key), entry.style);

	obs_property_t *secondary = obs_properties_add_list(
		cur, kCursorSecondaryStyle, obs_module_text("Cursor.SecondaryStyle"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(secondary, obs_module_text("Cursor.SecondarySame"), -1);
	for (const auto &entry : kCursorCatalog)
		obs_property_list_add_int(secondary, obs_module_text(entry.label_key), entry.style);
	set_tip(secondary, obs_module_text("Tooltip.CursorSecondary"));
	obs_property_t *link_detection =
		obs_properties_add_bool(cur, kCursorLinkDetection, obs_module_text("Cursor.LinkDetection"));
	set_tip(link_detection, obs_module_text("Tooltip.CursorLinkDetection"));

	add_range_slider(
		cur, kCursorSize, obs_module_text("Cursor.Size"),
		limits::kCursorSizeRange);
	obs_property_t *rotation = add_range_slider(
		cur, kCursorRotationOffset, obs_module_text("Cursor.RotationOffset"),
		limits::kCursorRotationRange);
	set_tip(rotation, obs_module_text("Tooltip.CursorRotationOffset"));
	obs_property_t *follow = obs_properties_add_list(
		cur, kCursorFollowMode, obs_module_text("Cursor.FollowMode"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(follow, obs_module_text("Cursor.Follow.Exact"),
				  static_cast<int>(CursorFollowMode::Exact));
	obs_property_list_add_int(follow, obs_module_text("Cursor.Follow.GlideAB"),
				  static_cast<int>(CursorFollowMode::GlideAB));
	obs_property_list_add_int(follow, obs_module_text("Cursor.Follow.Smooth"),
				  static_cast<int>(CursorFollowMode::Smooth));
	obs_property_set_modified_callback(follow, cursor_options_changed);
	add_range_slider(
		cur, kCursorGlideMs, obs_module_text("Cursor.GlideMs"),
		limits::kCursorGlideRange);
	obs_property_t *smooth_ms = add_range_slider(
		cur, kCursorSmoothMs, obs_module_text("Cursor.SmoothMs"),
		limits::kCursorSmoothRange);
	set_tip(smooth_ms, obs_module_text("Tooltip.CursorSmoothMs"));
	obs_properties_add_bool(cur, kCursorShadow, obs_module_text("Cursor.Shadow"));
	obs_property_t *hide_idle =
		obs_properties_add_bool(cur, kCursorHideIdle, obs_module_text("Cursor.HideIdle"));
	add_range_slider(
		cur, kCursorIdleDelayMs, obs_module_text("Cursor.IdleDelayMs"),
		limits::kCursorIdleDelayRange);
	obs_property_set_modified_callback(hide_idle, cursor_options_changed);
	obs_property_t *cur_group = obs_properties_add_group(
		pointer, kCursorEnabled, obs_module_text("Group.Cursor"), OBS_GROUP_CHECKABLE, cur);
	obs_property_set_modified_callback(cur_group, cursor_options_changed);
	cursor_options_changed(props, nullptr, settings);
}

void add_clicks_section(obs_properties_t *props, obs_properties_t *pointer, obs_data_t *settings)
{
	/* ---------------- Clicks ---------------- */
	add_click_group(pointer, "grp_click_left", obs_module_text("Group.ClickLeft"), kClickLeftEffect,
			kClickLeftSize, kClickLeftColor, kClickLeftDurationMs);
	add_click_group(pointer, "grp_click_right", obs_module_text("Group.ClickRight"), kClickRightEffect,
			kClickRightSize, kClickRightColor, kClickRightDurationMs);
	if (obs_property_t *p = obs_properties_get(props, kClickLeftEffect))
		obs_property_set_modified_callback(p, click_left_effect_changed);
	if (obs_property_t *p = obs_properties_get(props, kClickRightEffect))
		obs_property_set_modified_callback(p, click_right_effect_changed);
	click_left_effect_changed(props, nullptr, settings);
	click_right_effect_changed(props, nullptr, settings);
}

void add_click_sound_section(obs_properties_t *props, obs_properties_t *pointer, obs_data_t *settings)
{
	/* ---------------- Click sound ---------------- */
	obs_properties_t *snd = obs_properties_create();
	obs_property_t *snd_on = obs_properties_add_bool(snd, kClickSoundEnabled, obs_module_text("ClickSound.Enabled"));
	obs_property_set_modified_callback(snd_on, click_sound_changed);
	obs_property_t *sndleft = obs_properties_add_list(snd, kClickSoundFileLeft,
							 obs_module_text("ClickSound.LeftSound"),
							 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_t *sndright = obs_properties_add_list(snd, kClickSoundFileRight,
							   obs_module_text("ClickSound.RightSound"),
							   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback(sndleft, click_sound_side_changed);
	obs_property_set_modified_callback(sndright, click_sound_side_changed);
	obs_property_list_add_string(sndleft, obs_module_text("ClickSound.Mute"), "");
	obs_property_list_add_string(sndright, obs_module_text("ClickSound.Mute"), "");
	for (const auto &s : kSounds) {
		obs_property_list_add_string(sndleft, s.label, s.path);
		obs_property_list_add_string(sndright, s.label, s.path);
	}
	obs_property_t *volume = add_range_slider(
		snd, kClickSoundVolume, obs_module_text("ClickSound.Volume"),
		limits::kUnitRange);
	set_tip(volume, obs_module_text("Tooltip.ClickSoundVolume"));
	obs_property_t *add_audio = obs_properties_add_button2(
		snd, "click_sound_add_btn", obs_module_text("ClickSound.AddAudioButton"),
		add_click_audio_clicked, nullptr);
	set_tip(add_audio, obs_module_text("Tooltip.ClickSoundAddAudio"));
	obs_property_t *add_audio_all = obs_properties_add_button2(
		snd, "click_sound_add_all_btn", obs_module_text("ClickSound.AddAudioAllButton"),
		add_click_audio_all_clicked, nullptr);
	set_tip(add_audio_all, obs_module_text("Tooltip.ClickSoundAddAudioAll"));
	obs_properties_add_group(pointer, "grp_sound", obs_module_text("Group.ClickSound"), OBS_GROUP_NORMAL, snd);
	click_sound_changed(props, nullptr, settings);
}

} // namespace topscreen::properties
