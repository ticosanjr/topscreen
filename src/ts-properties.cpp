#include "ts-properties.hpp"

#include "core/easing.hpp"
#include "core/cursor_catalog.hpp"
#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "core/sound_catalog.hpp"
#include "core/types.hpp"
#include "ts-profiles-ui.hpp"
#include "ts-version.hpp"
#include "ts-wallpapers.hpp"

#include <string>

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen {

namespace {

/* Built-in fonts (data/fonts). */
const char *const kFonts[][2] = {
	{"DejaVu Sans Bold", "fonts/DejaVuSans-Bold.ttf"},
	{"DejaVu Sans", "fonts/DejaVuSans.ttf"},
	{"DejaVu Sans ExtraLight", "fonts/DejaVuSans-ExtraLight.ttf"},
	{"DejaVu Sans Mono Bold", "fonts/DejaVuSansMono-Bold.ttf"},
	{"DejaVu Sans Mono", "fonts/DejaVuSansMono.ttf"},
	{"DejaVu Serif Bold", "fonts/DejaVuSerif-Bold.ttf"},
};

/* Image selector filter (hardcoded, as in OBS: avoids escaping risk). */
const char *const kImageFilter =
	"Images (*.png *.jpg *.jpeg *.bmp *.tga *.gif *.webp *.PNG *.JPG *.JPEG *.BMP *.TGA *.GIF *.WEBP)"
	";;All files (*.*)";

/* Profiles backup files. */
const char *const kJsonFilter = "TopScreen profiles (*.json);;All files (*.*)";

void set_tip(obs_property_t *prop, const char *description)
{
	if (prop)
		obs_property_set_long_description(prop, description);
}

bool setting_bool(obs_data_t *settings, const char *key, bool fallback)
{
	return settings ? obs_data_get_bool(settings, key) : fallback;
}

long long setting_int(obs_data_t *settings, const char *key, long long fallback)
{
	return settings ? obs_data_get_int(settings, key) : fallback;
}

/* Wallpaper and PC Image are mutually exclusive. */
bool wallpaper_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)props;
	(void)p;
	const char *v = obs_data_get_string(settings, kBgImage);
	if (v && *v)
		obs_data_set_string(settings, kBgImageFile, "");
	return true;
}

bool bg_file_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)props;
	(void)p;
	const char *v = obs_data_get_string(settings, kBgImageFile);
	if (v && *v)
		obs_data_set_string(settings, kBgImage, "");
	return true;
}

bool background_type_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	int type = settings ? static_cast<int>(obs_data_get_int(settings, kBgType))
			    : static_cast<int>(BgMode::Gradient);
	if (type < static_cast<int>(BgMode::Solid) ||
	    type > static_cast<int>(BgMode::Wallpaper))
		type = static_cast<int>(BgMode::Gradient);

	const bool gradient = type == static_cast<int>(BgMode::Gradient);
	const bool wallpaper = type == static_cast<int>(BgMode::Wallpaper);

	if (obs_property_t *v = obs_properties_get(props, kBgColorA))
		obs_property_set_visible(v, !wallpaper);
	if (obs_property_t *v = obs_properties_get(props, kBgColorB))
		obs_property_set_visible(v, gradient);
	if (obs_property_t *v = obs_properties_get(props, kBgGradientAngle))
		obs_property_set_visible(v, gradient);
	if (obs_property_t *v = obs_properties_get(props, kBgGradientCenterX))
		obs_property_set_visible(v, gradient);
	if (obs_property_t *v = obs_properties_get(props, kBgGradientCenterY))
		obs_property_set_visible(v, gradient);
	if (obs_property_t *v = obs_properties_get(props, "grp_bg_image"))
		obs_property_set_visible(v, wallpaper);
	return true;
}

/* Cursor: the delay only matters while the cursor can hide, and the loop
 * duration only matters while the loop is enabled. */
bool cursor_options_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool hide = setting_bool(settings, kCursorHideIdle, true);
	const bool loop = setting_bool(settings, kCursorLoop, false);
	const long long mode =
		setting_int(settings, kCursorFollowMode,
			    static_cast<long long>(CursorFollowMode::Spring));
	const bool spring = mode == static_cast<long long>(CursorFollowMode::Spring);
	const bool glide = mode == static_cast<long long>(CursorFollowMode::GlideSampled) ||
			   mode == static_cast<long long>(CursorFollowMode::GlideAB);
	const bool sampled = mode == static_cast<long long>(CursorFollowMode::GlideSampled);
	if (obs_property_t *v = obs_properties_get(props, kCursorSmooth))
		obs_property_set_visible(v, spring);
	if (obs_property_t *v = obs_properties_get(props, kCursorGlideMs))
		obs_property_set_visible(v, glide);
	if (obs_property_t *v = obs_properties_get(props, kCursorGlideSampleMs))
		obs_property_set_visible(v, sampled);
	if (obs_property_t *v = obs_properties_get(props, kCursorIdleDelayMs))
		obs_property_set_visible(v, hide);
	if (obs_property_t *v = obs_properties_get(props, kCursorLoopMs))
		obs_property_set_visible(v, loop);
	return true;
}

/* Spotlight: the corner radius only applies to the square shape. */
bool spotlight_options_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const long long shape =
		setting_int(settings, kSpotlightShape, static_cast<long long>(SpotlightShape::Circle));
	if (obs_property_t *v = obs_properties_get(props, kSpotlightCornerRadius))
		obs_property_set_visible(v, shape == static_cast<long long>(SpotlightShape::Square));
	if (setting_bool(settings, kSpotlightEnabled, false) &&
	    setting_bool(settings, kMagnifierEnabled, false))
		obs_data_set_bool(settings, kMagnifierEnabled, false);
	return true;
}

/* Magnifier: same shape controls as spotlight, plus fixed-center visibility. */
bool magnifier_options_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const long long shape =
		setting_int(settings, kMagnifierShape, static_cast<long long>(SpotlightShape::Circle));
	const bool follow = setting_bool(settings, kMagnifierFollowCursor, true);
	if (obs_property_t *v = obs_properties_get(props, kMagnifierCornerRadius))
		obs_property_set_visible(v, shape == static_cast<long long>(SpotlightShape::Square));
	if (obs_property_t *v = obs_properties_get(props, kMagnifierCenterX))
		obs_property_set_visible(v, !follow);
	if (obs_property_t *v = obs_properties_get(props, kMagnifierCenterY))
		obs_property_set_visible(v, !follow);
	if (setting_bool(settings, kMagnifierEnabled, false) &&
	    setting_bool(settings, kSpotlightEnabled, false))
		obs_data_set_bool(settings, kSpotlightEnabled, false);
	return true;
}

/* Keystrokes: the band color only applies while the band is enabled. */
bool keys_options_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool band = setting_bool(settings, kKeysBand, true);
	if (obs_property_t *v = obs_properties_get(props, kKeysBandColor))
		obs_property_set_visible(v, band);
	return true;
}

/* Profiles backup buttons (Properties fallback; the dock has its own dialogs). */
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

void add_click_group(obs_properties_t *props, const char *group_key, const char *title,
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

	obs_properties_add_float_slider(g, size_key, obs_module_text("Click.Size"), 0.1, 4.0, 0.05);
	obs_properties_add_color(g, color_key, obs_module_text("Click.Color"));
	obs_properties_add_float_slider(g, duration_key, obs_module_text("Click.DurationMs"), 100.0, 2000.0, 10.0);

	obs_property_t *easing = obs_properties_add_list(g, easing_key, obs_module_text("Click.Easing"),
							 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(easing, obs_module_text("Easing.Linear"), EASE_LINEAR);
	obs_property_list_add_int(easing, obs_module_text("Easing.InCubic"), EASE_IN_CUBIC);
	obs_property_list_add_int(easing, obs_module_text("Easing.OutCubic"), EASE_OUT_CUBIC);
	obs_property_list_add_int(easing, obs_module_text("Easing.InOutCubic"), EASE_IN_OUT_CUBIC);

	obs_properties_add_group(props, group_key, title, OBS_GROUP_NORMAL, g);
}

} // namespace

/* Shows only the highlighter controls that apply to the current drag mode.
 * The callback receives the top-level tree, and obs_properties_get() searches
 * nested groups, so this works for the sub-groups below as well. */
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
	const long long mode = settings ? obs_data_get_int(settings, kHighlightMode)
					: static_cast<long long>(HighlightMode::Box);
	const bool hotkey_zoom = settings
					 ? obs_data_get_bool(settings, kHighlightHotkeyEnabled)
					 : true;
	/* The pure zoom mode draws no mark at all, so its look/timing controls do
	 * not apply. Every zoom mode (zoom, box+zoom, line+zoom) needs the zoom
	 * group; the independent hotkey also uses it in any drag mode. */
	const bool draws_mark = mode_draws_mark(static_cast<int>(mode));
	highlight_group_visible(props, "grp_hl_look", draws_mark);
	highlight_group_visible(props, "grp_hl_timing", draws_mark);
	const bool wants_zoom = mode_wants_zoom(static_cast<int>(mode)) || hotkey_zoom;
	highlight_group_visible(props, "grp_hl_zoom", wants_zoom);

	/* PressZoom is press-only: it uses its own short delay and the drag delay
	 * has no meaning there (and vice versa). */
	const bool press = mode_is_press_zoom(static_cast<int>(mode));
	if (obs_property_t *p = obs_properties_get(props, kHighlightZoomDelayMs))
		obs_property_set_visible(p, wants_zoom && !press);
	if (obs_property_t *p = obs_properties_get(props, kHighlightPressDelayMs))
		obs_property_set_visible(p, press);
	return true;
}

obs_properties_t *properties_create(void *data, obs_data_t *settings)
{
	obs_properties_t *props = obs_properties_create();

	/* ---------------- Version (always visible, first thing) ---------------- */
	const std::string version_line = std::string("TopScreen ") + kVersion;
	obs_properties_add_text(props, "topscreen_version", version_line.c_str(), OBS_TEXT_INFO);

	/* ---------------- Profiles (at the top) ---------------- */
	obs_properties_t *prof = obs_properties_create();
	obs_property_t *plist = obs_properties_add_list(prof, kProfileCurrent, obs_module_text("Profile.Current"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	/* Labels coming from the core: always "Profile A".."Profile J" (never repeated,
	 * even if the language file is missing/outdated). */
	for (int i = 0; i < kProfileCount; ++i)
		obs_property_list_add_int(plist, profile_label(i), i);
	obs_properties_add_text(prof, "profile_hint", obs_module_text("Profile.Hint"), OBS_TEXT_INFO);

	/* Backup / transfer: the OBS path widgets provide the native file chooser,
	 * so this fallback works on builds without the Qt dock. */
	obs_properties_t *backup = obs_properties_create();
	obs_properties_add_text(backup, "profile_backup_hint", obs_module_text("Profile.BackupHint"),
				OBS_TEXT_INFO);
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
	obs_properties_add_group(prof, "grp_profile_backup", obs_module_text("Profile.Backup"),
				 OBS_GROUP_NORMAL, backup);

	obs_properties_add_group(props, "grp_profiles", obs_module_text("Group.Profiles"), OBS_GROUP_NORMAL, prof);

	obs_properties_add_bool(props, kEffectsEnabled, obs_module_text("Enabled"));

	/* ---------------- Background ---------------- */
	obs_properties_t *bg = obs_properties_create();
	obs_properties_add_bool(bg, kBgEnabled, obs_module_text("Background.Enabled"));

	obs_property_t *mode = obs_properties_add_list(bg, kBgType,
						       obs_module_text("Background.Type"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mode, obs_module_text("Background.Type.Solid"),
				  static_cast<int>(BgMode::Solid));
	obs_property_list_add_int(mode, obs_module_text("Background.Type.Gradient"),
				  static_cast<int>(BgMode::Gradient));
	obs_property_list_add_int(mode, obs_module_text("Background.Type.Wallpaper"),
				  static_cast<int>(BgMode::Wallpaper));
	obs_property_set_modified_callback(mode, background_type_changed);

	obs_properties_add_color(bg, kBgColorA, obs_module_text("Background.ColorA"));
	obs_properties_add_color(bg, kBgColorB, obs_module_text("Background.ColorB"));
	obs_property_t *gangle = obs_properties_add_float_slider(
		bg, kBgGradientAngle, obs_module_text("Background.GradientAngle"), 0.0, 360.0,
		1.0);
	set_tip(gangle, obs_module_text("Tooltip.GradientAngle"));
	obs_property_t *gcenterx = obs_properties_add_float_slider(
		bg, kBgGradientCenterX, obs_module_text("Background.GradientCenterX"), 0.0, 1.0,
		0.01);
	set_tip(gcenterx, obs_module_text("Tooltip.GradientCenter"));
	obs_property_t *gcentery = obs_properties_add_float_slider(
		bg, kBgGradientCenterY, obs_module_text("Background.GradientCenterY"), 0.0, 1.0,
		0.01);
	set_tip(gcentery, obs_module_text("Tooltip.GradientCenter"));

	obs_properties_t *bgimg = obs_properties_create();
	/* NON-editable list: shows the label, saves the path. */
	obs_property_t *wp = obs_properties_add_list(bgimg, kBgImage,
						     obs_module_text("Background.Image"),
						     OBS_COMBO_TYPE_LIST,
						     OBS_COMBO_FORMAT_STRING);
	set_tip(wp, obs_module_text("Tooltip.WallpaperPreset"));
	obs_property_set_modified_callback(wp, wallpaper_modified);
	obs_property_list_add_string(wp, obs_module_text("Background.Image.Transparent"),
				     kTransparentWallpaper);
	obs_property_list_add_string(wp, obs_module_text("Background.Image.None"), "");
	for (const auto &entry : kWallpapers)
		obs_property_list_add_string(wp, entry.name, entry.path);

	obs_property_t *fp = obs_properties_add_path(
		bgimg, kBgImageFile, obs_module_text("Background.ImageFile"), OBS_PATH_FILE,
		kImageFilter, nullptr);
	set_tip(fp, obs_module_text("Tooltip.WallpaperFile"));
	obs_property_set_modified_callback(fp, bg_file_modified);
	obs_property_t *blur_amount = obs_properties_add_float_slider(
		bgimg, kBgImageBlur, obs_module_text("Background.ImageBlur"), 0.0, 60.0, 1.0);
	set_tip(blur_amount, obs_module_text("Tooltip.WallpaperBlur"));
	obs_property_t *bt = obs_properties_add_list(
		bgimg, kBgImageBlurType, obs_module_text("Background.ImageBlurType"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	set_tip(bt, obs_module_text("Tooltip.WallpaperDiffusion"));
	obs_property_list_add_int(bt, obs_module_text("Blur.None"), 0);
	obs_property_list_add_int(bt, obs_module_text("Blur.Soft"), 1);
	obs_property_list_add_int(bt, obs_module_text("Blur.Gaussian"), 2);
	obs_property_list_add_int(bt, obs_module_text("Blur.Creamy"), 3);
	obs_property_list_add_int(bt, obs_module_text("Blur.Frosted"), 4);
	obs_properties_add_group(bg, "grp_bg_image", obs_module_text("Background.ImageGroup"),
				 OBS_GROUP_NORMAL, bgimg);

	obs_properties_add_float_slider(bg, kBgPaddingX, obs_module_text("Background.PaddingX"), 0.0, 400.0, 1.0);
	obs_properties_add_float_slider(bg, kBgPaddingY, obs_module_text("Background.PaddingY"), 0.0, 400.0, 1.0);
	obs_properties_add_bool(bg, kBgKeepAspect, obs_module_text("Background.KeepAspect"));
	obs_properties_add_float_slider(bg, kBgCornerRadius, obs_module_text("Background.CornerRadius"), 0.0, 200.0,
					1.0);

	/* Window shadow (sub-group with clear names) */
	obs_properties_t *sh = obs_properties_create();
	obs_properties_add_bool(sh, kBgShadow, obs_module_text("Shadow.Enabled"));
	obs_property_t *shadow_alpha = obs_properties_add_float_slider(
		sh, kBgShadowAlpha, obs_module_text("Shadow.Opacity"), 0.0, 1.0, 0.01);
	set_tip(shadow_alpha, obs_module_text("Tooltip.ShadowOpacity"));
	obs_properties_add_float_slider(sh, kBgShadowBlur, obs_module_text("Shadow.Blur"), 0.0, 200.0, 1.0);
	obs_properties_add_float_slider(sh, kBgShadowOffsetX, obs_module_text("Shadow.OffsetX"), -100.0, 100.0,
					1.0);
	obs_properties_add_float_slider(sh, kBgShadowOffset, obs_module_text("Shadow.Offset"), -100.0, 100.0,
					1.0);
	obs_properties_add_group(bg, "grp_shadow", obs_module_text("Group.Shadow"), OBS_GROUP_NORMAL, sh);
	obs_properties_add_group(props, "grp_bg", obs_module_text("Group.Background"), OBS_GROUP_NORMAL, bg);
	background_type_changed(props, nullptr, settings);

	/* ---------------- Cursor ---------------- */
	obs_properties_t *cur = obs_properties_create();
	obs_properties_add_bool(cur, kCursorEnabled, obs_module_text("Cursor.Enabled"));

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

	obs_properties_add_float_slider(cur, kCursorSize, obs_module_text("Cursor.Size"), 0.5, 4.0, 0.05);
	obs_property_t *follow = obs_properties_add_list(
		cur, kCursorFollowMode, obs_module_text("Cursor.FollowMode"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(follow, obs_module_text("Cursor.Follow.Exact"),
				  static_cast<int>(CursorFollowMode::Exact));
	obs_property_list_add_int(follow, obs_module_text("Cursor.Follow.Spring"),
				  static_cast<int>(CursorFollowMode::Spring));
	obs_property_list_add_int(follow, obs_module_text("Cursor.Follow.GlideSampled"),
				  static_cast<int>(CursorFollowMode::GlideSampled));
	obs_property_list_add_int(follow, obs_module_text("Cursor.Follow.GlideAB"),
				  static_cast<int>(CursorFollowMode::GlideAB));
	obs_property_set_modified_callback(follow, cursor_options_changed);
	obs_properties_add_float_slider(cur, kCursorSmooth, obs_module_text("Cursor.Smooth"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(cur, kCursorGlideMs, obs_module_text("Cursor.GlideMs"), 50.0, 1200.0, 10.0);
	obs_properties_add_float_slider(cur, kCursorGlideSampleMs,
					obs_module_text("Cursor.GlideSampleMs"), 20.0, 500.0, 5.0);
	obs_properties_add_float_slider(cur, kCursorSway, obs_module_text("Cursor.Sway"), 0.0, 1.0, 0.01);
	obs_properties_add_bool(cur, kCursorShadow, obs_module_text("Cursor.Shadow"));
	obs_property_t *hide_idle =
		obs_properties_add_bool(cur, kCursorHideIdle, obs_module_text("Cursor.HideIdle"));
	obs_properties_add_float_slider(cur, kCursorIdleDelayMs, obs_module_text("Cursor.IdleDelayMs"), 0.0,
					5000.0, 50.0);
	obs_property_t *loop =
		obs_properties_add_bool(cur, kCursorLoop, obs_module_text("Cursor.Loop"));
	set_tip(loop, obs_module_text("Tooltip.CursorLoop"));
	obs_property_t *loop_ms = obs_properties_add_float_slider(
		cur, kCursorLoopMs, obs_module_text("Cursor.LoopMs"), 1000.0, 20000.0, 100.0);
	set_tip(loop_ms, obs_module_text("Tooltip.CursorLoopMs"));
	obs_property_set_modified_callback(hide_idle, cursor_options_changed);
	obs_property_set_modified_callback(loop, cursor_options_changed);
	obs_properties_add_group(props, "grp_cursor", obs_module_text("Group.Cursor"), OBS_GROUP_NORMAL, cur);
	cursor_options_changed(props, nullptr, settings);

	/* ---------------- Clicks ---------------- */
	add_click_group(props, "grp_click_left", obs_module_text("Group.ClickLeft"), kClickLeftEffect,
			kClickLeftSize, kClickLeftColor, kClickLeftDurationMs, kClickLeftEasing);
	add_click_group(props, "grp_click_right", obs_module_text("Group.ClickRight"), kClickRightEffect,
			kClickRightSize, kClickRightColor, kClickRightDurationMs, kClickRightEasing);

	/* ---------------- Click sound ---------------- */
	obs_properties_t *snd = obs_properties_create();
	obs_properties_add_bool(snd, kClickSoundEnabled, obs_module_text("ClickSound.Enabled"));
	obs_property_t *sndleft = obs_properties_add_list(snd, kClickSoundFileLeft,
							 obs_module_text("ClickSound.LeftSound"),
							 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_t *sndright = obs_properties_add_list(snd, kClickSoundFileRight,
							   obs_module_text("ClickSound.RightSound"),
							   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	for (const auto &s : kSounds) {
		obs_property_list_add_string(sndleft, s.label, s.path);
		obs_property_list_add_string(sndright, s.label, s.path);
	}
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
	obs_property_t *pan = obs_properties_add_float_slider(
		zoom, kZoomPanAmount, obs_module_text("Zoom.PanAmount"), 0.0, 1.0, 0.01);
	set_tip(pan, obs_module_text("Tooltip.ZoomPan"));
	obs_property_t *center = obs_properties_add_float_slider(
		zoom, kZoomCenterAmount, obs_module_text("Zoom.CenterAmount"), 0.0, 1.0, 0.01);
	set_tip(center, obs_module_text("Tooltip.ZoomCentering"));
	obs_properties_add_group(props, "grp_zoom", obs_module_text("Group.Zoom"), OBS_GROUP_NORMAL, zoom);

	/* ---------------- Motion blur ---------------- */
	obs_properties_t *mb = obs_properties_create();
	obs_properties_add_bool(mb, kMotionBlurEnabled, obs_module_text("MotionBlur.Enabled"));
	obs_properties_add_float_slider(mb, kMotionBlurAmount, obs_module_text("MotionBlur.Amount"), 0.0, 1.0,
					0.01);
	obs_properties_add_group(props, "grp_motionblur", obs_module_text("Group.MotionBlur"), OBS_GROUP_NORMAL, mb);

	/* ---------------- Spotlight ---------------- */
	obs_properties_t *spot = obs_properties_create();
	obs_property_t *spot_enabled =
		obs_properties_add_bool(spot, kSpotlightEnabled, obs_module_text("Spotlight.Enabled"));
	obs_property_set_modified_callback(spot_enabled, spotlight_options_changed);

	obs_property_t *sshape = obs_properties_add_list(
		spot, kSpotlightShape, obs_module_text("Spotlight.Shape"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(sshape, obs_module_text("Spotlight.Shape.Circle"),
				  static_cast<int>(core::SpotlightShape::Circle));
	obs_property_list_add_int(sshape, obs_module_text("Spotlight.Shape.Square"),
				  static_cast<int>(core::SpotlightShape::Square));
	obs_property_set_modified_callback(sshape, spotlight_options_changed);

	obs_properties_add_float_slider(spot, kSpotlightRadius, obs_module_text("Spotlight.Radius"),
					0.05, 1.0, 0.01);
	obs_properties_add_float_slider(spot, kSpotlightCornerRadius,
					obs_module_text("Spotlight.CornerRadius"), 0.0, 200.0, 1.0);
	obs_property_t *dim = obs_properties_add_float_slider(
		spot, kSpotlightDim, obs_module_text("Spotlight.Dim"), 0.0, 1.0, 0.01);
	set_tip(dim, obs_module_text("Tooltip.SpotlightDim"));

	obs_properties_t *sborder = obs_properties_create();
	obs_properties_add_bool(sborder, kSpotlightBorderEnabled,
				obs_module_text("Spotlight.BorderEnabled"));
	obs_properties_add_color(sborder, kSpotlightBorderColor,
				 obs_module_text("Spotlight.BorderColor"));
	obs_properties_add_float_slider(sborder, kSpotlightBorderWidth,
					obs_module_text("Spotlight.BorderWidth"), 0.5, 40.0, 0.5);
	obs_properties_add_group(spot, "grp_spotlight_border", obs_module_text("Spotlight.BorderGroup"),
				 OBS_GROUP_NORMAL, sborder);

	obs_properties_t *sshadow = obs_properties_create();
	obs_properties_add_bool(sshadow, kSpotlightShadowEnabled,
				obs_module_text("Spotlight.ShadowEnabled"));
	obs_properties_add_color(sshadow, kSpotlightShadowColor,
				 obs_module_text("Spotlight.ShadowColor"));
	obs_properties_add_float_slider(sshadow, kSpotlightShadowAlpha,
					obs_module_text("Spotlight.ShadowAlpha"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(sshadow, kSpotlightShadowBlur,
					obs_module_text("Spotlight.ShadowBlur"), 0.0, 200.0, 1.0);
	obs_properties_add_float_slider(sshadow, kSpotlightShadowOffsetX,
					obs_module_text("Spotlight.ShadowOffsetX"), -100.0, 100.0,
					1.0);
	obs_properties_add_float_slider(sshadow, kSpotlightShadowOffsetY,
					obs_module_text("Spotlight.ShadowOffsetY"), -100.0, 100.0,
					1.0);
	obs_properties_add_group(spot, "grp_spotlight_shadow", obs_module_text("Spotlight.ShadowGroup"),
				 OBS_GROUP_NORMAL, sshadow);

	obs_properties_t *sblur = obs_properties_create();
	obs_properties_add_bool(sblur, kSpotlightBlurEnabled, obs_module_text("Spotlight.BlurEnabled"));
	obs_properties_add_float_slider(sblur, kSpotlightBlurAmount,
					obs_module_text("Spotlight.BlurAmount"), 0.0, 60.0, 1.0);
	obs_properties_add_group(spot, "grp_spotlight_blur", obs_module_text("Spotlight.BlurGroup"),
				 OBS_GROUP_NORMAL, sblur);

	obs_properties_t *sanim = obs_properties_create();
	obs_properties_add_float_slider(sanim, kSpotlightZoomAmount,
					obs_module_text("Spotlight.ZoomAmount"), 1.0, 4.0, 0.05);
	obs_properties_add_float_slider(sanim, kSpotlightInMs, obs_module_text("Spotlight.InMs"),
					0.0, 2000.0, 10.0);
	obs_properties_add_float_slider(sanim, kSpotlightOutMs, obs_module_text("Spotlight.OutMs"),
					0.0, 2000.0, 10.0);
	obs_property_t *drop = obs_properties_add_float_slider(
		sanim, kSpotlightDropPx, obs_module_text("Spotlight.DropPx"), 0.0, 200.0, 1.0);
	set_tip(drop, obs_module_text("Tooltip.SpotlightDrop"));
	obs_properties_add_group(spot, "grp_spotlight_animation",
				 obs_module_text("Spotlight.AnimationGroup"), OBS_GROUP_NORMAL,
				 sanim);

	obs_properties_add_group(props, "grp_spotlight", obs_module_text("Group.Spotlight"), OBS_GROUP_NORMAL,
				 spot);
	spotlight_options_changed(props, nullptr, settings);

	/* ---------------- Magnifier ---------------- */
	obs_properties_t *mag = obs_properties_create();
	obs_property_t *mag_enabled =
		obs_properties_add_bool(mag, kMagnifierEnabled, obs_module_text("Magnifier.Enabled"));
	obs_property_t *mshape = obs_properties_add_list(
		mag, kMagnifierShape, obs_module_text("Magnifier.Shape"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mshape, obs_module_text("Magnifier.Shape.Circle"),
				  static_cast<int>(SpotlightShape::Circle));
	obs_property_list_add_int(mshape, obs_module_text("Magnifier.Shape.Square"),
				  static_cast<int>(SpotlightShape::Square));
	obs_property_set_modified_callback(mshape, magnifier_options_changed);
	obs_properties_add_float_slider(mag, kMagnifierRadius, obs_module_text("Magnifier.Radius"),
					0.05, 1.0, 0.01);
	obs_properties_add_float_slider(mag, kMagnifierZoom, obs_module_text("Magnifier.Zoom"),
					1.0, 4.0, 0.05);
	obs_properties_add_float_slider(mag, kMagnifierCornerRadius,
					obs_module_text("Magnifier.CornerRadius"), 0.0, 200.0, 1.0);
	obs_property_t *magnifier_follow =
		obs_properties_add_bool(mag, kMagnifierFollowCursor, obs_module_text("Magnifier.FollowCursor"));
	obs_property_set_modified_callback(magnifier_follow, magnifier_options_changed);
	obs_properties_add_float_slider(mag, kMagnifierCenterX, obs_module_text("Magnifier.CenterX"),
					0.0, 1.0, 0.01);
	obs_properties_add_float_slider(mag, kMagnifierCenterY, obs_module_text("Magnifier.CenterY"),
					0.0, 1.0, 0.01);
	obs_properties_t *mborder = obs_properties_create();
	obs_properties_add_bool(mborder, kMagnifierBorderEnabled,
				obs_module_text("Magnifier.BorderEnabled"));
	obs_properties_add_color(mborder, kMagnifierBorderColor,
				 obs_module_text("Magnifier.BorderColor"));
	obs_properties_add_float_slider(mborder, kMagnifierBorderWidth,
					obs_module_text("Magnifier.BorderWidth"), 0.5, 40.0, 0.5);
	obs_properties_add_group(mag, "grp_magnifier_border", obs_module_text("Magnifier.BorderGroup"),
				 OBS_GROUP_NORMAL, mborder);
	obs_properties_t *mshadow = obs_properties_create();
	obs_properties_add_bool(mshadow, kMagnifierShadowEnabled,
				obs_module_text("Magnifier.ShadowEnabled"));
	obs_properties_add_color(mshadow, kMagnifierShadowColor,
				 obs_module_text("Magnifier.ShadowColor"));
	obs_properties_add_float_slider(mshadow, kMagnifierShadowAlpha,
					obs_module_text("Magnifier.ShadowAlpha"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(mshadow, kMagnifierShadowBlur,
					obs_module_text("Magnifier.ShadowBlur"), 0.0, 200.0, 1.0);
	obs_properties_add_float_slider(mshadow, kMagnifierShadowOffsetX,
					obs_module_text("Magnifier.ShadowOffsetX"), -100.0, 100.0, 1.0);
	obs_properties_add_float_slider(mshadow, kMagnifierShadowOffsetY,
					obs_module_text("Magnifier.ShadowOffsetY"), -100.0, 100.0, 1.0);
	obs_properties_add_group(mag, "grp_magnifier_shadow", obs_module_text("Magnifier.ShadowGroup"),
				 OBS_GROUP_NORMAL, mshadow);
	obs_properties_add_float_slider(mag, kMagnifierInMs, obs_module_text("Magnifier.InMs"),
					0.0, 2000.0, 10.0);
	obs_properties_add_float_slider(mag, kMagnifierOutMs, obs_module_text("Magnifier.OutMs"),
					0.0, 2000.0, 10.0);
	obs_properties_add_float_slider(mag, kMagnifierDropPx, obs_module_text("Magnifier.DropPx"),
					0.0, 200.0, 1.0);
	obs_properties_add_group(props, "grp_magnifier", obs_module_text("Group.Magnifier"),
				 OBS_GROUP_NORMAL, mag);
	obs_property_set_modified_callback(mag_enabled, magnifier_options_changed);
	magnifier_options_changed(props, nullptr, settings);

	/* ---------------- Keys ---------------- */
	obs_properties_t *keys = obs_properties_create();
	obs_properties_add_bool(keys, kKeysEnabled, obs_module_text("Keys.Enabled"));
	obs_properties_add_bool(keys, kKeysModifiersOnly, obs_module_text("Keys.ModifiersOnly"));
	obs_properties_add_bool(keys, kKeysShowSpecial, obs_module_text("Keys.ShowSpecial"));
	obs_properties_add_float_slider(keys, kKeysTtlMs, obs_module_text("Keys.TtlMs"), 500.0, 8000.0, 50.0);
	obs_properties_add_int_slider(keys, kKeysMax, obs_module_text("Keys.Max"), 1, 12, 1);
	obs_properties_add_float_slider(keys, kKeysPosX, obs_module_text("Keys.PosX"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(keys, kKeysPosY, obs_module_text("Keys.PosY"), 0.0, 1.0, 0.01);
	obs_properties_add_float_slider(keys, kKeysSize, obs_module_text("Keys.Size"), 0.4, 4.0, 0.05);

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
	obs_properties_add_group(props, "grp_keys", obs_module_text("Group.Keys"), OBS_GROUP_NORMAL, keys);
	keys_options_changed(props, nullptr, settings);

	/* ---------------- Highlighter: action first, then only what applies.
	 * The mode list below shows/hides the Look / Timing / Zoom groups. */
	obs_properties_t *hl = obs_properties_create();
	obs_properties_add_bool(hl, kHighlightEnabled, obs_module_text("Highlight.Enabled"));
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
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.PressZoom"),
				  static_cast<int>(HighlightMode::PressZoom));
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.BoxZoom"),
				  static_cast<int>(HighlightMode::BoxZoom));
	obs_property_list_add_int(hmode, obs_module_text("Highlight.Mode.LineZoom"),
				  static_cast<int>(HighlightMode::LineZoom));
	obs_property_set_modified_callback(hmode, highlight_mode_changed);
	obs_property_set_modified_callback(hkh, highlight_mode_changed);

	/* Look (box + line). */
	obs_properties_t *hl_look = obs_properties_create();
	obs_properties_add_color(hl_look, kHighlightColor, obs_module_text("Highlight.Color"));
	obs_properties_add_float_slider(hl_look, kHighlightOpacity, obs_module_text("Highlight.Opacity"),
					0.0, 1.0, 0.01);
	obs_properties_add_float_slider(hl_look, kHighlightThickness,
					obs_module_text("Highlight.Thickness"), 0.001, 0.05, 0.001);
	obs_properties_add_group(hl, "grp_hl_look", obs_module_text("Highlight.Look"), OBS_GROUP_NORMAL,
				 hl_look);

	/* Timing (box + line): how long the mark stays and fades. */
	obs_properties_t *hl_timing = obs_properties_create();
	obs_properties_add_float_slider(hl_timing, kHighlightHoldMs, obs_module_text("Highlight.HoldMs"),
					0.0, 5000.0, 50.0);
	obs_properties_add_float_slider(hl_timing, kHighlightFadeMs, obs_module_text("Highlight.FadeMs"),
					0.0, 2000.0, 50.0);
	obs_properties_add_group(hl, "grp_hl_timing", obs_module_text("Highlight.Timing"), OBS_GROUP_NORMAL,
				 hl_timing);

	/* Zoom gesture: only in Zoom mode. */
	obs_properties_t *hl_zoom = obs_properties_create();
	obs_property_t *zdir = obs_properties_add_list(hl_zoom, kHighlightZoomDir,
						       obs_module_text("Highlight.ZoomDir"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(zdir, obs_module_text("Highlight.ZoomDir.In"), 0);
	obs_property_list_add_int(zdir, obs_module_text("Highlight.ZoomDir.Out"), 1);
	obs_property_list_add_int(zdir, obs_module_text("Highlight.ZoomDir.PunchIn"), 2);
	obs_property_list_add_int(zdir, obs_module_text("Highlight.ZoomDir.PunchOut"), 3);

	obs_property_t *zoom_delay = obs_properties_add_float_slider(
		hl_zoom, kHighlightZoomDelayMs, obs_module_text("Highlight.ZoomDelayMs"), 0.0,
		5000.0, 50.0);
	set_tip(zoom_delay, obs_module_text("Tooltip.HighlightZoomDelay"));
	obs_property_t *press_delay = obs_properties_add_float_slider(
		hl_zoom, kHighlightPressDelayMs, obs_module_text("Highlight.PressDelayMs"), 0.0,
		3000.0, 50.0);
	set_tip(press_delay, obs_module_text("Tooltip.HighlightPressDelay"));
	obs_properties_add_float_slider(hl_zoom, kHighlightZoomMax, obs_module_text("Highlight.ZoomMax"),
					1.2, 8.0, 0.1);
	obs_properties_add_float_slider(hl_zoom, kHighlightZoomInMs,
					obs_module_text("Highlight.ZoomInMs"), 50.0, 2000.0, 10.0);
	obs_properties_add_float_slider(hl_zoom, kHighlightZoomOutMs, obs_module_text("Highlight.ZoomOutMs"),
					100.0, 8000.0, 50.0);
	obs_properties_add_float_slider(hl_zoom, kHighlightZoomHoldMs,
					obs_module_text("Highlight.ZoomHoldMs"), 0.0, 5000.0,
					50.0);
	obs_properties_add_group(hl, "grp_hl_zoom", obs_module_text("Highlight.ZoomGroup"), OBS_GROUP_NORMAL,
				 hl_zoom);

	obs_properties_add_group(props, "grp_highlight", obs_module_text("Group.Highlight"), OBS_GROUP_NORMAL,
				 hl);

	/* Initial visibility matches the saved mode (the callback keeps it live). */
	highlight_mode_changed(props, nullptr, settings);

	/* ---------------- Monitor origin ---------------- */
	obs_properties_t *mon = obs_properties_create();
	obs_properties_add_text(mon, "monitor_hint", obs_module_text("Monitor.Hint"), OBS_TEXT_INFO);
	obs_properties_add_bool(mon, kMonitorManual, obs_module_text("Monitor.Manual"));
	obs_properties_add_int(mon, kMonitorX, obs_module_text("Monitor.X"), -32768, 32767, 1);
	obs_properties_add_int(mon, kMonitorY, obs_module_text("Monitor.Y"), -32768, 32767, 1);
	obs_properties_add_group(props, "grp_monitor", obs_module_text("Group.Monitor"), OBS_GROUP_NORMAL,
				 mon);

	/* ---------------- Profiles ---------------- */
	return props;
}

} // namespace topscreen
