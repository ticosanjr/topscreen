#include "ts-properties.hpp"

#include "properties/ts-option-schema.hpp"
#include "properties/ts-properties-internal.hpp"

#include "core/config_limits.hpp"
#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "core/types.hpp"
#include "hotkeys/ts-hotkeys.hpp"
#include "ts-profile-store.hpp"
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
	if (!profiles_overwrite_confirmed(data)) {
		blog(LOG_WARNING, "[TopScreen] profiles import: tick the overwrite confirmation first");
		return false;
	}
	return profiles_import_from_properties(data);
}

bool reset_profiles_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	if (!profiles_overwrite_confirmed(data)) {
		blog(LOG_WARNING,
		     "[TopScreen] profiles reset: tick the overwrite confirmation first");
		return false;
	}
	return profiles_reset_from_properties(data);
}

bool rename_profile_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return profiles_rename_from_properties(data);
}

bool duplicate_profile_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return profiles_duplicate_from_properties(data);
}

bool revert_profile_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return profiles_revert_from_properties(data);
}

bool default_hotkeys_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(data);
	Hotkeys::apply_default_profile_bindings();
	return false;
}

bool scene_switch_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	scene_profiles::set_enabled(obs_data_get_bool(settings, kSceneSwitchEnabled));
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

void add_profiles_section(obs_properties_t *props, void *data, obs_data_t *settings)
{
	/* ---------------- Profiles (at the top) ---------------- */
	obs_properties_t *prof = obs_properties_create();
	obs_property_t *plist = obs_properties_add_list(prof, kProfileCurrent, obs_module_text("Profile.Current"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	/* Custom names are stored in the slot; fall back to the stable A..J label. */
	for (int i = 0; i < kProfileCount; ++i) {
		const std::string label = settings ? profile_store::display_name(settings, i)
						   : profile_label(i);
		obs_property_list_add_int(plist, label.c_str(), i);
	}
	obs_property_t *name_input = obs_properties_add_text(
		prof, kProfileNameInput, obs_module_text("Profile.Name"), OBS_TEXT_DEFAULT);
	set_tip(name_input, obs_module_text("Tooltip.ProfileName"));
	obs_property_t *rename_btn = obs_properties_add_button2(
		prof, "profile_rename_btn", obs_module_text("Profile.RenameButton"),
		rename_profile_clicked, data);
	set_tip(rename_btn, obs_module_text("Tooltip.ProfileRename"));

	const int current = settings ? static_cast<int>(obs_data_get_int(settings, kProfileCurrent)) : 0;
	obs_property_t *dup = obs_properties_add_list(
		prof, kProfileDuplicateTarget, obs_module_text("Profile.Duplicate"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	for (int i = 0; i < kProfileCount; ++i) {
		if (i == current)
			continue;
		const std::string label = settings ? profile_store::display_name(settings, i)
						   : profile_label(i);
		obs_property_list_add_int(dup, label.c_str(), i);
	}
	obs_property_t *dup_btn = obs_properties_add_button2(
		prof, "profile_duplicate_btn", obs_module_text("Profile.DuplicateButton"),
		duplicate_profile_clicked, data);
	set_tip(dup_btn, obs_module_text("Tooltip.ProfileDuplicate"));
	obs_property_t *revert_btn = obs_properties_add_button2(
		prof, "profile_revert_btn", obs_module_text("Profile.RevertButton"),
		revert_profile_clicked, data);
	set_tip(revert_btn, obs_module_text("Tooltip.ProfileRevert"));

	obs_properties_add_text(prof, "profile_hint", obs_module_text("Profile.Hint"), OBS_TEXT_INFO);
	obs_properties_add_text(prof, "profile_scene_hint", obs_module_text("Profile.SceneHint"),
				OBS_TEXT_INFO);
	if (settings)
		obs_data_set_bool(settings, kSceneSwitchEnabled, scene_profiles::enabled());
	obs_property_t *scene_switch = obs_properties_add_bool(
		prof, kSceneSwitchEnabled, obs_module_text("Profile.SceneSwitch"));
	obs_property_set_modified_callback(scene_switch, scene_switch_changed);
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
	obs_property_t *confirm = obs_properties_add_bool(
		backup, kProfileConfirmOverwrite, obs_module_text("Profile.ConfirmOverwrite"));
	set_tip(confirm, obs_module_text("Tooltip.ProfileConfirmOverwrite"));
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
	/* One registry drives every help tooltip. Missing keys (e.g. the filter-only
	 * monitor group on a capture source) are simply skipped. */
	for (const auto &spec : options::kSpecs) {
		if (obs_property_t *p = obs_properties_get(props, spec.key))
			set_tip(p, obs_module_text(spec.tooltip));
	}
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
	add_profiles_section(props, data, settings);

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
	properties::add_click_sound_section(props, pointer, settings, data);
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
