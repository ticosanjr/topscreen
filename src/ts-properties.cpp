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
	properties::add_profiles_section(props, data, settings);

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
	properties::add_profile_backup_section(props, data);
	add_tooltips(props);

	/* Version last: useful for support, but not the first thing to read. */
	const std::string version_line = std::string("TopScreen ") + kVersion;
	obs_properties_add_text(props, "topscreen_version", version_line.c_str(), OBS_TEXT_INFO);
	return props;
}

} // namespace topscreen
