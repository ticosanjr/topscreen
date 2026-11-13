#include "properties/ts-properties-internal.hpp"

#include "core/config.hpp"
#include "core/config_limits.hpp"
#include "core/easing.hpp"
#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "core/types.hpp"

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen::properties {

/* Built-in fonts (data/fonts). */
const char *const kFonts[][2] = {
	{"DejaVu Sans Bold", "fonts/DejaVuSans-Bold.ttf"},
	{"DejaVu Sans", "fonts/DejaVuSans.ttf"},
	{"DejaVu Sans ExtraLight", "fonts/DejaVuSans-ExtraLight.ttf"},
	{"DejaVu Sans Mono Bold", "fonts/DejaVuSansMono-Bold.ttf"},
	{"DejaVu Sans Mono", "fonts/DejaVuSansMono.ttf"},
	{"DejaVu Serif Bold", "fonts/DejaVuSerif-Bold.ttf"},
};

bool keys_options_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kKeysEnabled, true);
	const bool band = setting_bool(settings, kKeysBand, true);
	for (const char *key : {kKeysModifiersOnly, kKeysShowSpecial, kKeysShowShortcuts,
			       kKeysTtlMs, kKeysMax, kKeysPosX, kKeysPosY, kKeysSize,
			       kKeysFont, kKeysColor, kKeysShadow, kKeysBand,
			       kKeysBandColor}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	if (obs_property_t *v = obs_properties_get(props, kKeysBandColor))
		obs_property_set_visible(v, on && band);
	return true;
}

bool highlight_group_visible(obs_properties_t *props, const char *group, bool visible)
{
	obs_property_t *p = obs_properties_get(props, group);
	if (!p)
		return false;
	obs_property_set_visible(p, visible);
	return true;
}

bool highlight_mode_changed(obs_properties_t *props, obs_property_t *property, obs_data_t *settings)
{
	(void)property;
	const bool enabled = settings ? obs_data_get_bool(settings, kHighlightEnabled)
				      : true;
	const long long mode = settings ? obs_data_get_int(settings, kHighlightMode)
					: static_cast<long long>(HighlightMode::Box);
	const bool hotkey_zoom = settings
					 ? obs_data_get_bool(settings, kHighlightHotkeyEnabled)
					 : true;
	const bool lock = settings ? obs_data_get_bool(settings, kHighlightLockUntilClick)
				   : false;
	const bool reading = mode_reads(static_cast<int>(mode));
	/* The pure zoom and Reading modes draw no mark at all, so look/timing do
	 * not apply. A pinned mark also ignores hold/fade: it stays until the next
	 * click. Every zoom mode and Reading need the zoom group; the independent
	 * hotkey also uses it in any drag mode. */
	const bool draws_mark = enabled && mode_draws_mark(static_cast<int>(mode));
	highlight_group_visible(props, "grp_hl_look", draws_mark);
	highlight_group_visible(props, "grp_hl_timing", draws_mark && !lock);
	highlight_group_visible(props, "grp_hl_reading", enabled && reading);
	const bool wants_zoom = enabled && (mode_wants_zoom(static_cast<int>(mode)) ||
					    hotkey_zoom || reading);
	highlight_group_visible(props, "grp_hl_zoom", wants_zoom);
	if (obs_property_t *p = obs_properties_get(props, kHighlightMode))
		obs_property_set_visible(p, enabled);
	if (obs_property_t *p = obs_properties_get(props, kHighlightHotkeyEnabled))
		obs_property_set_visible(p, enabled);
	if (obs_property_t *p = obs_properties_get(props, kHighlightLockUntilClick))
		obs_property_set_visible(p, enabled && !reading);
	return true;
}

void add_highlight_section(obs_properties_t *props, obs_properties_t *overlays, obs_data_t *settings)
{
	/* ---------------- Highlighter: action first, then only what applies.
	 * The mode list below shows/hides the Look / Timing / Zoom groups. */
	obs_properties_t *hl = obs_properties_create();
	obs_property_t *hl_on =
		obs_properties_add_bool(hl, kHighlightEnabled, obs_module_text("Highlight.Enabled"));
	obs_property_set_modified_callback(hl_on, highlight_mode_changed);
	obs_property_t *hkh = obs_properties_add_bool(hl, kHighlightHotkeyEnabled,
						      obs_module_text("Highlight.HotkeyEnabled"));

	obs_property_t *hmode = obs_properties_add_list(hl, kHighlightMode,
							obs_module_text("Highlight.Mode"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.Box"),
				  static_cast<int>(HighlightMode::Box));
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.Line"),
				  static_cast<int>(HighlightMode::Line));
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.Zoom"),
				  static_cast<int>(HighlightMode::Zoom));
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.BoxZoom"),
				  static_cast<int>(HighlightMode::BoxZoom));
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.LineZoom"),
				  static_cast<int>(HighlightMode::LineZoom));
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.Reading"),
				  static_cast<int>(HighlightMode::Reading));
	obs_property_set_modified_callback(hmode, highlight_mode_changed);
	obs_property_set_modified_callback(hkh, highlight_mode_changed);

	obs_property_t *lock = obs_properties_add_bool(
		hl, kHighlightLockUntilClick, obs_module_text("Highlight.LockUntilClick"));
	set_tip(lock, obs_module_text("Tooltip.HighlightLockUntilClick"));
	obs_property_set_modified_callback(lock, highlight_mode_changed);

	/* Look (box + line). */
	obs_properties_t *hl_look = obs_properties_create();
	obs_properties_add_color(hl_look, kHighlightColor, obs_module_text("Highlight.Color"));
	add_range_slider(
		hl_look, kHighlightOpacity, obs_module_text("Highlight.Opacity"),
		limits::kUnitRange);
	add_range_slider(
		hl_look, kHighlightThickness, obs_module_text("Highlight.Thickness"),
		limits::kHighlightThicknessRange);
	obs_properties_add_group(hl, "grp_hl_look", obs_module_text("Highlight.Look"), OBS_GROUP_NORMAL,
				 hl_look);

	/* Timing (box + line): how long the mark stays and fades. */
	obs_properties_t *hl_timing = obs_properties_create();
	add_range_slider(
		hl_timing, kHighlightHoldMs, obs_module_text("Highlight.HoldMs"),
		limits::kHighlightHoldRange);
	add_range_slider(
		hl_timing, kHighlightFadeMs, obs_module_text("Highlight.FadeMs"),
		limits::kHighlightFadeRange);
	obs_properties_add_group(hl, "grp_hl_timing", obs_module_text("Highlight.Timing"), OBS_GROUP_NORMAL,
				 hl_timing);

	/* Reading: dim the area outside the selected rectangle. */
	obs_properties_t *hl_reading = obs_properties_create();
	obs_property_t *reading_dim = add_range_slider(
		hl_reading, kHighlightReadingDim, obs_module_text("Highlight.ReadingDim"),
		limits::kUnitRange);
	set_tip(reading_dim, obs_module_text("Tooltip.HighlightReadingDim"));
	obs_properties_add_group(hl, "grp_hl_reading", obs_module_text("Highlight.ReadingGroup"),
				 OBS_GROUP_NORMAL, hl_reading);

	/* Zoom gesture: one fixed IN sequence with ease-in-out cubic. */
	obs_properties_t *hl_zoom = obs_properties_create();
	obs_property_t *zoom_delay = add_range_slider(
		hl_zoom, kHighlightZoomDelayMs, obs_module_text("Highlight.ZoomDelayMs"),
		limits::kHighlightHoldRange);
	set_tip(zoom_delay, obs_module_text("Tooltip.HighlightZoomDelay"));
	add_range_slider(
		hl_zoom, kHighlightZoomMax, obs_module_text("Highlight.ZoomMax"),
		limits::kHighlightZoomMaxRange);
	add_range_slider(
		hl_zoom, kHighlightZoomInMs, obs_module_text("Highlight.ZoomInMs"),
		limits::kHighlightZoomInRange);
	add_range_slider(
		hl_zoom, kHighlightZoomHoldMs, obs_module_text("Highlight.ZoomHoldMs"),
		limits::kHighlightHoldRange);
	add_range_slider(
		hl_zoom, kHighlightZoomOutMs, obs_module_text("Highlight.ZoomOutMs"),
		limits::kHighlightZoomOutRange);
	obs_properties_add_group(hl, "grp_hl_zoom", obs_module_text("Highlight.ZoomGroup"), OBS_GROUP_NORMAL,
				 hl_zoom);

	obs_properties_add_group(overlays, "grp_highlight", obs_module_text("Group.Highlight"), OBS_GROUP_NORMAL,
				 hl);

	/* Initial visibility matches the saved mode (the callback keeps it live). */
	highlight_mode_changed(props, nullptr, settings);
}

void add_keys_section(obs_properties_t *props, obs_properties_t *overlays, obs_data_t *settings)
{
	/* ---------------- Keys ---------------- */
	obs_properties_t *keys = obs_properties_create();
	obs_property_t *keys_on =
		obs_properties_add_bool(keys, kKeysEnabled, obs_module_text("Keys.Enabled"));
	obs_property_set_modified_callback(keys_on, keys_options_changed);
	obs_properties_add_bool(keys, kKeysModifiersOnly, obs_module_text("Keys.ModifiersOnly"));
	obs_properties_add_bool(keys, kKeysShowSpecial, obs_module_text("Keys.ShowSpecial"));
	obs_properties_add_bool(keys, kKeysShowShortcuts, obs_module_text("Keys.ShowShortcuts"));
	add_range_slider(
		keys, kKeysTtlMs, obs_module_text("Keys.TtlMs"),
		limits::kKeysTtlRange);
	add_range_int_slider(
		keys, kKeysMax, obs_module_text("Keys.Max"),
		limits::kKeysMaxRange);
	add_range_slider(
		keys, kKeysPosX, obs_module_text("Keys.PosX"),
		limits::kUnitRange);
	add_range_slider(
		keys, kKeysPosY, obs_module_text("Keys.PosY"),
		limits::kUnitRange);
	add_range_slider(
		keys, kKeysSize, obs_module_text("Keys.Size"),
		limits::kKeysSizeRange);

	obs_property_t *kfont = obs_properties_add_list(keys, kKeysFont, obs_module_text("Keys.Font"),
							OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
	for (const auto &f : kFonts)
		obs_property_list_add_string(kfont, f[0], f[1]);

	obs_properties_add_color(keys, kKeysColor, obs_module_text("Keys.Color"));
	obs_properties_add_bool(keys, kKeysShadow, obs_module_text("Keys.Shadow"));
	obs_property_t *band =
		obs_properties_add_bool(keys, kKeysBand, obs_module_text("Keys.Band"));
	obs_properties_add_color(keys, kKeysBandColor, obs_module_text("Keys.BandColor"));
	obs_property_set_modified_callback(band, keys_options_changed);

	obs_properties_add_group(overlays, "grp_keys", obs_module_text("Group.Keys"), OBS_GROUP_NORMAL, keys);
	keys_options_changed(props, nullptr, settings);
}

} // namespace topscreen::properties
