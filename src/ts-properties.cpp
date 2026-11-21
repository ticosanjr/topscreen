#include "ts-properties.hpp"

#include "properties/ts-properties-internal.hpp"

#include "core/config_limits.hpp"
#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "core/types.hpp"
#include "hotkeys/ts-hotkeys.hpp"
#include "ts-profiles-ui.hpp"
#include "ts-registry.hpp"
#include "ts-scene-profiles.hpp"
#include "ts-version.hpp"
#include "webcam/ts-webcam-source.hpp"

#include <obs-frontend-api.h>

#include <string>

using namespace topscreen::core;
using namespace topscreen::core::keys;
using namespace topscreen::properties;

namespace topscreen {

namespace {

/* Profiles backup files. */
const char *const kJsonFilter = "TopScreen profiles (*.json);;All files (*.*)";

bool export_profiles_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return profiles_export_from_properties(data);
}

bool import_profiles_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return profiles_import_from_properties(data);
}

bool reset_profiles_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return profiles_reset_from_properties(data);
}

bool default_hotkeys_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(data);
	Hotkeys::apply_default_profile_bindings();
	return false;
}

bool assign_scene_profile_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	(void)props;
	(void)p;
	auto *control = static_cast<FilterControl *>(data);
	obs_source_t *scene = obs_frontend_get_current_scene();
	if (!control || !scene) {
		if (scene)
			obs_source_release(scene);
		return false;
	}
	const bool ok = scene_profiles::assign(scene, control->current_profile());
	obs_source_release(scene);
	return ok;
}

bool clear_scene_profile_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	(void)props;
	(void)p;
	(void)data;
	obs_source_t *scene = obs_frontend_get_current_scene();
	if (!scene)
		return false;
	const bool ok = scene_profiles::clear(scene);
	obs_source_release(scene);
	return ok;
}

bool clear_all_scene_profiles_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	(void)props;
	(void)p;
	(void)data;
	return scene_profiles::clear_all();
}

void add_profiles_section(obs_properties_t *props, void *data)
{
	/* ---------------- Profiles (at the top) ---------------- */
	obs_properties_t *prof = obs_properties_create();
	obs_property_t *plist = obs_properties_add_list(prof, kProfileCurrent, obs_module_text("Profile.Current"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	/* Labels coming from the core: always "Profile A".."Profile J" (never repeated,
	 * even if the language file is missing/outdated). */
	for (int i = 0; i < kProfileCount; ++i)
		obs_property_list_add_int(plist, profile_label(i), i);
	obs_properties_add_text(prof, "profile_hint", obs_module_text("Profile.Hint"), OBS_TEXT_INFO);
	obs_properties_add_text(prof, "profile_scene_hint", obs_module_text("Profile.SceneHint"),
				OBS_TEXT_INFO);
	std::string scene_list;
	for (const auto &entry : scene_profiles::list()) {
		scene_list += entry.first;
		scene_list += " -> ";
		scene_list += profile_label(entry.second);
		scene_list += '\n';
	}
	if (scene_list.empty())
		scene_list = obs_module_text("Profile.SceneListNone");
	obs_properties_add_text(prof, "profile_scene_list", scene_list.c_str(), OBS_TEXT_INFO);
	obs_property_t *assign_scene = obs_properties_add_button2(
		prof, "profile_assign_scene", obs_module_text("Profile.AssignSceneButton"),
		assign_scene_profile_clicked, data);
	set_tip(assign_scene, obs_module_text("Tooltip.ProfileAssignScene"));
	obs_property_t *clear_scene = obs_properties_add_button2(
		prof, "profile_clear_scene", obs_module_text("Profile.ClearSceneButton"),
		clear_scene_profile_clicked, data);
	set_tip(clear_scene, obs_module_text("Tooltip.ProfileClearScene"));
	obs_property_t *clear_all_scenes = obs_properties_add_button2(
		prof, "profile_clear_all_scenes", obs_module_text("Profile.ClearAllScenesButton"),
		clear_all_scene_profiles_clicked, data);
	set_tip(clear_all_scenes, obs_module_text("Tooltip.ProfileClearAllScenes"));

	obs_properties_add_group(props, "grp_profiles", obs_module_text("Group.Profiles"), OBS_GROUP_NORMAL, prof);
}

void add_profile_backup_section(obs_properties_t *props, void *data)
{
	/* Backup / transfer: the OBS path widgets provide the native file chooser,
	 * so this fallback works on builds without the Qt dock. */
	obs_properties_t *backup = obs_properties_create();
	obs_properties_add_text(backup, "profile_backup_hint", obs_module_text("Profile.BackupHint"),
				OBS_TEXT_INFO);
	obs_property_t *warning = obs_properties_add_text(
		backup, "profile_backup_warning", obs_module_text("Profile.BackupWarning"),
		OBS_TEXT_INFO);
	obs_property_text_set_info_type(warning, OBS_TEXT_INFO_WARNING);
	obs_property_text_set_info_word_wrap(warning, true);
	obs_properties_add_path(backup, kProfileExportPath, obs_module_text("Profile.ExportPath"),
				OBS_PATH_FILE_SAVE, kJsonFilter, nullptr);
	obs_properties_add_button2(backup, "profile_export_btn",
				   obs_module_text("Profile.ExportButton"), export_profiles_clicked,
				   data);
	obs_properties_add_path(backup, kProfileImportPath, obs_module_text("Profile.ImportPath"),
				OBS_PATH_FILE, kJsonFilter, nullptr);
	obs_properties_add_button2(backup, "profile_import_btn",
				   obs_module_text("Profile.ImportButton"), import_profiles_clicked,
				   data);
	obs_property_t *reset_btn = obs_properties_add_button2(
		backup, "profile_reset_btn", obs_module_text("Profile.ResetButton"),
		reset_profiles_clicked, data);
	set_tip(reset_btn, obs_module_text("Tooltip.ProfileReset"));
	obs_property_t *hotkey_btn = obs_properties_add_button2(
		backup, "profile_hotkeys_btn", obs_module_text("Profile.DefaultHotkeysButton"),
		default_hotkeys_clicked, data);
	set_tip(hotkey_btn, obs_module_text("Tooltip.ProfileDefaultHotkeys"));

	obs_properties_add_group(props, "grp_profile_backup", obs_module_text("Profile.Backup"),
				 OBS_GROUP_NORMAL, backup);
}

bool add_webcam_source_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	(void)props;
	(void)p;
	(void)data;
	return webcam_source::add_bubble_source();
}

void add_webcam_section(obs_properties_t *props, obs_properties_t *overlays)
{
	obs_properties_t *webcam = obs_properties_create();
	obs_properties_add_text(webcam, "webcam_main_hint", obs_module_text("Webcam.AddHint"),
				OBS_TEXT_INFO);
	obs_property_t *add = obs_properties_add_button2(
		webcam, "webcam_add_source", obs_module_text("Webcam.AddButton"),
		add_webcam_source_clicked, nullptr);
	set_tip(add, obs_module_text("Webcam.Tip.Add"));
	obs_properties_add_group(overlays, "grp_webcam", obs_module_text("Webcam.MainGroup"),
				 OBS_GROUP_NORMAL, webcam);
	(void)props;
}

void add_monitor_section(obs_properties_t *advanced)
{
	/* ---------------- Monitor origin ---------------- */
	obs_properties_t *mon = obs_properties_create();
	obs_properties_add_text(mon, "monitor_hint", obs_module_text("Monitor.Hint"), OBS_TEXT_INFO);
	obs_properties_add_bool(mon, kMonitorManual, obs_module_text("Monitor.Manual"));
	add_range_int(
		mon, kMonitorX, obs_module_text("Monitor.X"),
		limits::kMonitorPositionRange);
	add_range_int(
		mon, kMonitorY, obs_module_text("Monitor.Y"),
		limits::kMonitorPositionRange);
	obs_properties_add_group(advanced, "grp_monitor", obs_module_text("Group.Monitor"), OBS_GROUP_NORMAL,
				 mon);
}

void add_tooltips(obs_properties_t *props)
{
	/* Simple English explanations shown as the OBS "?" tooltip. */
	auto tip = [&](const char *name, const char *text) {
		if (obs_property_t *p = obs_properties_get(props, name))
			set_tip(p, text);
	};
	tip("grp_profiles", obs_module_text("Tooltip.GroupProfiles"));
	tip("grp_profile_backup", obs_module_text("Tooltip.GroupProfileBackup"));
	tip(kBgEnabled, obs_module_text("Tooltip.GroupBackground"));
	tip("grp_bg_image", obs_module_text("Tooltip.GroupWallpaper"));
	tip(kBgShadow, obs_module_text("Tooltip.GroupShadow"));
	tip("grp_frame", obs_module_text("Tooltip.GroupFrame"));
	tip("grp_webcam", obs_module_text("Tooltip.GroupWebcam"));
	tip(kCursorEnabled, obs_module_text("Tooltip.GroupCursor"));
	tip("grp_click_left", obs_module_text("Tooltip.GroupClickLeft"));
	tip("grp_click_right", obs_module_text("Tooltip.GroupClickRight"));
	tip("grp_sound", obs_module_text("Tooltip.GroupClickSound"));
	tip("grp_zoom", obs_module_text("Tooltip.GroupZoom"));
	tip(kSpotlightEnabled, obs_module_text("Tooltip.GroupSpotlight"));
	tip("grp_spotlight_border", obs_module_text("Tooltip.GroupSpotlightBorder"));
	tip("grp_spotlight_blur", obs_module_text("Tooltip.GroupSpotlightBlur"));
	tip("grp_spotlight_animation", obs_module_text("Tooltip.GroupSpotlightAnimation"));
	tip(kMagnifierEnabled, obs_module_text("Tooltip.GroupMagnifier"));
	tip("grp_magnifier_border", obs_module_text("Tooltip.GroupMagnifierBorder"));
	tip("grp_magnifier_shadow", obs_module_text("Tooltip.GroupMagnifierShadow"));
	tip(kKeysEnabled, obs_module_text("Tooltip.GroupKeys"));
	tip(kHighlightEnabled, obs_module_text("Tooltip.GroupHighlight"));
	tip("grp_hl_look", obs_module_text("Tooltip.GroupHighlightLook"));
	tip("grp_hl_timing", obs_module_text("Tooltip.GroupHighlightTiming"));
	tip("grp_hl_zoom", obs_module_text("Tooltip.GroupHighlightZoom"));
	tip("grp_monitor", obs_module_text("Tooltip.GroupMonitor"));
	tip(kEffectsEnabled, obs_module_text("Tooltip.EffectsEnabled"));
	tip(kBgType, obs_module_text("Tooltip.BgType"));
	tip(kBgPaddingX, obs_module_text("Tooltip.BgPaddingX"));
	tip(kBgPaddingY, obs_module_text("Tooltip.BgPaddingY"));
	tip(kBgKeepAspect, obs_module_text("Tooltip.BgKeepAspect"));
	tip(kBgCornerRadius, obs_module_text("Tooltip.BgCornerRadius"));
	tip(kCursorStyle, obs_module_text("Tooltip.CursorStyle"));
	tip(kCursorSecondaryStyle, obs_module_text("Tooltip.CursorSecondary"));
	tip(kCursorLinkDetection, obs_module_text("Tooltip.CursorLinkDetection"));
	tip(kCursorFollowMode, obs_module_text("Tooltip.CursorFollowMode"));
	tip(kCursorGlideMs, obs_module_text("Tooltip.CursorGlideMs"));
	tip(kCursorSize, obs_module_text("Tooltip.CursorSize"));
	tip(kCursorRotationOffset, obs_module_text("Tooltip.CursorRotationOffset"));
	tip(kCursorHideIdle, obs_module_text("Tooltip.CursorHideIdle"));
	tip(kClickLeftEffect, obs_module_text("Tooltip.ClickEffect"));
	tip(kClickLeftSize, obs_module_text("Tooltip.ClickSize"));
	tip(kClickLeftDurationMs, obs_module_text("Tooltip.ClickDuration"));
	tip(kClickSoundVolume, obs_module_text("Tooltip.ClickSoundVolume"));
	tip(kZoomMode, obs_module_text("Tooltip.ZoomMode"));
	tip(kZoomAmount, obs_module_text("Tooltip.ZoomAmount"));
	tip(kZoomIdleTimeoutMs, obs_module_text("Tooltip.ZoomIdleTimeout"));
	tip(kSpotlightOutsideMode, obs_module_text("Tooltip.SpotlightOutsideMode"));
	tip(kSpotlightShape, obs_module_text("Tooltip.SpotlightShape"));
	tip(kSpotlightRadius, obs_module_text("Tooltip.SpotlightRadius"));
	tip(kSpotlightDim, obs_module_text("Tooltip.SpotlightDim"));
	tip(kSpotlightZoomAmount, obs_module_text("Tooltip.SpotlightZoom"));
	tip(kMagnifierShape, obs_module_text("Tooltip.MagnifierShape"));
	tip(kMagnifierRadius, obs_module_text("Tooltip.MagnifierRadius"));
	tip(kMagnifierZoom, obs_module_text("Tooltip.MagnifierZoom"));
	tip(kMagnifierFollowCursor, obs_module_text("Tooltip.MagnifierFollow"));
	tip(kMagnifierInMs, obs_module_text("Tooltip.MagnifierIn"));
	tip(kMagnifierOutMs, obs_module_text("Tooltip.MagnifierOut"));
	tip(kKeysModifiersOnly, obs_module_text("Tooltip.KeysModifiersOnly"));
	tip(kKeysShowSpecial, obs_module_text("Tooltip.KeysShowSpecial"));
	tip(kKeysShowShortcuts, obs_module_text("Tooltip.KeysShowShortcuts"));
	tip(kKeysTtlMs, obs_module_text("Tooltip.KeysTtl"));
	tip(kKeysMax, obs_module_text("Tooltip.KeysMax"));
	tip(kKeysPosX, obs_module_text("Tooltip.KeysPosX"));
	tip(kKeysPosY, obs_module_text("Tooltip.KeysPosY"));
	tip(kKeysSize, obs_module_text("Tooltip.KeysSize"));
	tip(kKeysBand, obs_module_text("Tooltip.KeysBand"));
	tip(kHighlightMode, obs_module_text("Tooltip.HighlightMode"));
	tip(kHighlightOpacity, obs_module_text("Tooltip.HighlightOpacity"));
	tip(kHighlightHoldMs, obs_module_text("Tooltip.HighlightHold"));
	tip(kHighlightFadeMs, obs_module_text("Tooltip.HighlightFade"));
	tip(kMonitorManual, obs_module_text("Tooltip.MonitorManual"));
	/* Secondary tooltips for the remaining controls. */
	tip(kBgColorA, obs_module_text("Tooltip.kBgColorA"));
	tip(kBgColorB, obs_module_text("Tooltip.kBgColorB"));
	tip(kBgGradientAngle, obs_module_text("Tooltip.kBgGradientAngle"));
	tip(kBgGradientCenterX, obs_module_text("Tooltip.kBgGradientCenterX"));
	tip(kBgGradientCenterY, obs_module_text("Tooltip.kBgGradientCenterY"));
	tip(kBgImage, obs_module_text("Tooltip.kBgImage"));
	tip(kBgImageBlur, obs_module_text("Tooltip.kBgImageBlur"));
	tip(kBgImageFile, obs_module_text("Tooltip.kBgImageFile"));
	tip(kBgKeepAspect, obs_module_text("Tooltip.kBgKeepAspect"));
	tip(kBgShadowAlpha, obs_module_text("Tooltip.kBgShadowAlpha"));
	tip(kBgShadowBlur, obs_module_text("Tooltip.kBgShadowBlur"));
	tip(kBgShadowOffset, obs_module_text("Tooltip.kBgShadowOffset"));
	tip(kBgShadowOffsetX, obs_module_text("Tooltip.kBgShadowOffsetX"));
	tip(kClickSoundEnabled, obs_module_text("Tooltip.kClickSoundEnabled"));
	tip(kClickSoundFileLeft, obs_module_text("Tooltip.kClickSoundFileLeft"));
	tip(kClickSoundFileRight, obs_module_text("Tooltip.kClickSoundFileRight"));
	tip(kCursorIdleDelayMs, obs_module_text("Tooltip.kCursorIdleDelayMs"));
	tip(kCursorShadow, obs_module_text("Tooltip.kCursorShadow"));
	tip(kHighlightColor, obs_module_text("Tooltip.kHighlightColor"));
	tip(kHighlightHotkeyEnabled, obs_module_text("Tooltip.kHighlightHotkeyEnabled"));
	tip(kHighlightThickness, obs_module_text("Tooltip.kHighlightThickness"));
	tip(kHighlightZoomDelayMs, obs_module_text("Tooltip.kHighlightZoomDelayMs"));
	tip(kHighlightZoomHoldMs, obs_module_text("Tooltip.kHighlightZoomHoldMs"));
	tip(kHighlightZoomInMs, obs_module_text("Tooltip.kHighlightZoomInMs"));
	tip(kHighlightZoomMax, obs_module_text("Tooltip.kHighlightZoomMax"));
	tip(kHighlightZoomOutMs, obs_module_text("Tooltip.kHighlightZoomOutMs"));
	tip(kKeysBandColor, obs_module_text("Tooltip.kKeysBandColor"));
	tip(kKeysColor, obs_module_text("Tooltip.kKeysColor"));
	tip(kKeysFont, obs_module_text("Tooltip.kKeysFont"));
	tip(kKeysShadow, obs_module_text("Tooltip.kKeysShadow"));
	tip(kMagnifierBorderColor, obs_module_text("Tooltip.kMagnifierBorderColor"));
	tip(kMagnifierBorderEnabled, obs_module_text("Tooltip.kMagnifierBorderEnabled"));
	tip(kMagnifierBorderWidth, obs_module_text("Tooltip.kMagnifierBorderWidth"));
	tip(kMagnifierCenterX, obs_module_text("Tooltip.kMagnifierCenterX"));
	tip(kMagnifierCenterY, obs_module_text("Tooltip.kMagnifierCenterY"));
	tip(kMagnifierCornerRadius, obs_module_text("Tooltip.kMagnifierCornerRadius"));
	tip(kMagnifierShadowAlpha, obs_module_text("Tooltip.kMagnifierShadowAlpha"));
	tip(kMagnifierShadowBlur, obs_module_text("Tooltip.kMagnifierShadowBlur"));
	tip(kMagnifierShadowColor, obs_module_text("Tooltip.kMagnifierShadowColor"));
	tip(kMagnifierShadowEnabled, obs_module_text("Tooltip.kMagnifierShadowEnabled"));
	tip(kMagnifierShadowOffsetX, obs_module_text("Tooltip.kMagnifierShadowOffsetX"));
	tip(kMagnifierShadowOffsetY, obs_module_text("Tooltip.kMagnifierShadowOffsetY"));
	tip(kMonitorX, obs_module_text("Tooltip.kMonitorX"));
	tip(kMonitorY, obs_module_text("Tooltip.kMonitorY"));
	tip(kProfileCurrent, obs_module_text("Tooltip.kProfileCurrent"));
	tip(kProfileExportPath, obs_module_text("Tooltip.kProfileExportPath"));
	tip(kProfileImportPath, obs_module_text("Tooltip.kProfileImportPath"));
	tip(kSpotlightBlurAmount, obs_module_text("Tooltip.kSpotlightBlurAmount"));
	tip(kSpotlightBorderColor, obs_module_text("Tooltip.kSpotlightBorderColor"));
	tip(kSpotlightBorderEnabled, obs_module_text("Tooltip.kSpotlightBorderEnabled"));
	tip(kSpotlightBorderWidth, obs_module_text("Tooltip.kSpotlightBorderWidth"));
	tip(kSpotlightCornerRadius, obs_module_text("Tooltip.kSpotlightCornerRadius"));
	tip(kSpotlightInMs, obs_module_text("Tooltip.kSpotlightInMs"));
	tip(kSpotlightOutMs, obs_module_text("Tooltip.kSpotlightOutMs"));
	tip(kZoomCenterAmount, obs_module_text("Tooltip.kZoomCenterAmount"));
	tip(kZoomDurationMs, obs_module_text("Tooltip.kZoomDurationMs"));
	tip(kZoomPanAmount, obs_module_text("Tooltip.kZoomPanAmount"));
	tip("grp_screen", obs_module_text("Tooltip.GroupCapture"));
	tip(kSpotlightShowCursor, obs_module_text("Tooltip.SpotlightShowCursor"));
	tip(kMagnifierShowCursor, obs_module_text("Tooltip.MagnifierShowCursor"));
	tip("grp_pointer", obs_module_text("Tooltip.GroupPointer"));
	tip("grp_focus", obs_module_text("Tooltip.GroupFocus"));
	tip("grp_overlays", obs_module_text("Tooltip.GroupOverlays"));
	tip("grp_zoom_highlight", obs_module_text("Tooltip.GroupZoomHighlight"));
	tip("grp_advanced", obs_module_text("Tooltip.GroupAdvanced"));
}

} // namespace

obs_properties_t *properties_create(void *data, obs_data_t *settings, bool include_monitor,
				    properties_prefix_fn prefix, void *prefix_data)
{
	obs_properties_t *props = obs_properties_create();
	if (prefix)
		prefix(props, prefix_data);

	/* The master switch is the first thing a user looks for. */
	obs_properties_add_bool(props, kEffectsEnabled, obs_module_text("Enabled"));
	add_profiles_section(props, data);

	obs_properties_t *screen = obs_properties_create();
	obs_properties_t *zoom_highlight = obs_properties_create();
	obs_properties_t *pointer = obs_properties_create();
	obs_properties_t *focus = obs_properties_create();
	obs_properties_t *overlays = obs_properties_create();
	/* Only create the Advanced container when it will actually be added; an
	 * orphan properties object would leak. */
	obs_properties_t *advanced = include_monitor ? obs_properties_create() : nullptr;
	/* Top-level sections group the detailed settings by purpose. They are added
	 * before their children so the visibility callbacks can find nested groups. */
	obs_properties_add_group(props, "grp_screen", obs_module_text("Group.Capture"),
				 OBS_GROUP_NORMAL, screen);
	obs_properties_add_group(props, "grp_zoom_highlight", obs_module_text("Group.ZoomHighlight"),
				 OBS_GROUP_NORMAL, zoom_highlight);
	obs_properties_add_group(props, "grp_pointer", obs_module_text("Group.Pointer"),
				 OBS_GROUP_NORMAL, pointer);
	obs_properties_add_group(props, "grp_focus", obs_module_text("Group.Focus"),
				 OBS_GROUP_NORMAL, focus);
	obs_properties_add_group(props, "grp_overlays", obs_module_text("Group.Overlays"),
				 OBS_GROUP_NORMAL, overlays);
	/* The capture source hides the filter's monitor section, so it must not
	 * show an empty "Advanced" group either. */
	if (include_monitor)
		obs_properties_add_group(props, "grp_advanced", obs_module_text("Group.Advanced"),
					 OBS_GROUP_NORMAL, advanced);
	properties::add_background_section(props, screen, settings);
	properties::add_zoom_section(props, zoom_highlight, settings);
	properties::add_highlight_section(props, zoom_highlight, settings);
	properties::add_cursor_section(props, pointer, settings);
	properties::add_clicks_section(props, pointer, settings);
	properties::add_click_sound_section(props, pointer, settings);
	properties::add_spotlight_section(props, focus, settings);
	properties::add_magnifier_section(props, focus, settings);
	properties::add_keys_section(props, overlays, settings);
	add_webcam_section(props, overlays);

	if (include_monitor)
		add_monitor_section(advanced);
	add_profile_backup_section(props, data);
	add_tooltips(props);

	/* Version last: useful for support, but not the first thing to read. */
	const std::string version_line = std::string("TopScreen ") + kVersion;
	obs_properties_add_text(props, "topscreen_version", version_line.c_str(), OBS_TEXT_INFO);
	return props;
}

} // namespace topscreen
