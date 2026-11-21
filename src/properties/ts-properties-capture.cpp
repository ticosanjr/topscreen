#include "properties/ts-properties-internal.hpp"

#include "core/config.hpp"
#include "core/config_limits.hpp"
#include "core/easing.hpp"
#include "core/keys.hpp"
#include "core/types.hpp"
#include "ts-wallpapers.hpp"
#include <string>

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen::properties {

/* Used by the local-image selector. */
const char *const kImageFilter =
	"Images (*.png *.jpg *.jpeg *.bmp *.tga *.gif *.webp *.PNG *.JPG *.JPEG *.BMP *.TGA *.GIF *.WEBP)"
	";;All files (*.*)";

bool background_type_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings);

bool wallpaper_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	const char *v = obs_data_get_string(settings, kBgImage);
	if (v && *v) {
		obs_data_set_string(settings, kBgImageFile, "");
		obs_data_set_int(settings, kBgType, static_cast<int>(BgMode::Wallpaper));
		obs_data_set_int(settings, kBgMode, legacy_bg_mode_value(static_cast<int>(BgMode::Wallpaper)));
	}
	background_type_changed(props, p, settings);
	return true;
}

bool bg_file_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	const char *v = obs_data_get_string(settings, kBgImageFile);
	if (v && *v) {
		obs_data_set_string(settings, kBgImage, "");
		obs_data_set_int(settings, kBgType, static_cast<int>(BgMode::LocalImage));
		obs_data_set_int(settings, kBgMode, legacy_bg_mode_value(static_cast<int>(BgMode::LocalImage)));
	}
	background_type_changed(props, p, settings);
	return true;
}

bool background_type_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	int type = settings ? static_cast<int>(obs_data_get_int(settings, kBgType))
			    : static_cast<int>(BgMode::Gradient);
	if (type < static_cast<int>(BgMode::Solid) ||
	    type > static_cast<int>(BgMode::BlurredScreen))
		type = static_cast<int>(BgMode::Gradient);

	const bool solid = type == static_cast<int>(BgMode::Solid);
	const bool gradient = type == static_cast<int>(BgMode::Gradient);
	const bool wallpaper = type == static_cast<int>(BgMode::Wallpaper);
	const bool local = type == static_cast<int>(BgMode::LocalImage);
	const bool blurred = type == static_cast<int>(BgMode::BlurredScreen);
	const bool image = wallpaper || local;

	auto show = [&](const char *key, bool visible) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, visible);
	};
	show(kBgColorA, solid || gradient);
	show(kBgColorB, gradient);
	show(kBgGradientAngle, gradient);
	show(kBgGradientCenterX, gradient);
	show(kBgGradientCenterY, gradient);
	show("grp_bg_image", image || blurred);
	show(kBgImage, wallpaper);
	show(kBgImageFile, local);
	show(kBgImageBlur, image || blurred);
	/* Padding, corner and the window shadow still shape the transparent
	 * "floating screen", so they stay available for every background type. */
	return true;
}

bool frame_style_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = settings &&
			obs_data_get_int(settings, kFrameStyle) != 0;
	for (const char *key : {kFrameColor, kFrameWidth}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	return true;
}

bool background_enabled_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kBgEnabled, true);
	for (const char *key : {kBgType, kBgColorA, kBgColorB, kBgGradientAngle,
			       kBgGradientCenterX, kBgGradientCenterY, "grp_bg_image",
			       kBgPaddingX, kBgPaddingY, kBgKeepAspect, kBgCornerRadius,
			       kBgShadow, "grp_frame"}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	if (on)
		background_type_changed(props, nullptr, settings);
	return true;
}

bool background_shadow_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kBgShadow, true);
	for (const char *key : {kBgShadowAlpha, kBgShadowBlur, kBgShadowOffsetX,
			       kBgShadowOffset}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	return true;
}

void add_background_section(obs_properties_t *props, obs_properties_t *capture, obs_data_t *settings)
{
	/* ---------------- Background ---------------- */
	obs_properties_t *bg = obs_properties_create();
	obs_property_t *mode = obs_properties_add_list(bg, kBgType,
						       obs_module_text("Background.Type"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mode, obs_module_text("Background.Type.Solid"),
				  static_cast<int>(BgMode::Solid));
	obs_property_list_add_int(mode, obs_module_text("Background.Type.Gradient"),
				  static_cast<int>(BgMode::Gradient));
	obs_property_list_add_int(mode, obs_module_text("Background.Type.Wallpaper"),
				  static_cast<int>(BgMode::Wallpaper));
	obs_property_list_add_int(mode, obs_module_text("Background.Type.LocalImage"),
				  static_cast<int>(BgMode::LocalImage));
	obs_property_list_add_int(mode, obs_module_text("Background.Type.Transparent"),
				  static_cast<int>(BgMode::Transparent));
	obs_property_list_add_int(mode, obs_module_text("Background.Type.BlurredScreen"),
				  static_cast<int>(BgMode::BlurredScreen));
	obs_property_set_modified_callback(mode, background_type_changed);

	obs_properties_add_color(bg, kBgColorA, obs_module_text("Background.ColorA"));
	obs_properties_add_color(bg, kBgColorB, obs_module_text("Background.ColorB"));
	obs_property_t *gangle = add_range_slider(
		bg, kBgGradientAngle, obs_module_text("Background.GradientAngle"),
		limits::kBgGradientAngleRange);
	set_tip(gangle, obs_module_text("Tooltip.GradientAngle"));
	obs_property_t *gcenterx = add_range_slider(
		bg, kBgGradientCenterX, obs_module_text("Background.GradientCenterX"),
		limits::kUnitRange);
	set_tip(gcenterx, obs_module_text("Tooltip.GradientCenter"));
	obs_property_t *gcentery = add_range_slider(
		bg, kBgGradientCenterY, obs_module_text("Background.GradientCenterY"),
		limits::kUnitRange);
	set_tip(gcentery, obs_module_text("Tooltip.GradientCenter"));

	obs_properties_t *bgimg = obs_properties_create();
	/* NON-editable list: shows the label, saves the path. */
	obs_property_t *wp = obs_properties_add_list(bgimg, kBgImage,
						     obs_module_text("Background.Image"),
						     OBS_COMBO_TYPE_LIST,
						     OBS_COMBO_FORMAT_STRING);
	set_tip(wp, obs_module_text("Tooltip.WallpaperPreset"));
	obs_property_set_modified_callback(wp, wallpaper_modified);
	for (const auto &entry : kWallpapers)
		obs_property_list_add_string(wp, entry.name, entry.path);

	obs_property_t *fp = obs_properties_add_path(
		bgimg, kBgImageFile, obs_module_text("Background.ImageFile"), OBS_PATH_FILE,
		kImageFilter, nullptr);
	set_tip(fp, obs_module_text("Tooltip.WallpaperFile"));
	obs_property_set_modified_callback(fp, bg_file_modified);
	obs_property_t *blur_amount = add_range_slider(
		bgimg, kBgImageBlur, obs_module_text("Background.ImageBlur"),
		limits::kBgImageBlurRange);
	set_tip(blur_amount, obs_module_text("Tooltip.WallpaperBlur"));
	obs_properties_add_group(bg, "grp_bg_image", obs_module_text("Background.ImageGroup"),
				 OBS_GROUP_NORMAL, bgimg);

	add_range_slider(
		bg, kBgPaddingX, obs_module_text("Background.PaddingX"),
		limits::kBgPaddingRange);
	add_range_slider(
		bg, kBgPaddingY, obs_module_text("Background.PaddingY"),
		limits::kBgPaddingRange);
	obs_properties_add_bool(bg, kBgKeepAspect, obs_module_text("Background.KeepAspect"));
	add_range_slider(
		bg, kBgCornerRadius, obs_module_text("Background.CornerRadius"),
		limits::kCornerRadiusRange);

	/* Window shadow (sub-group with clear names) */
	obs_properties_t *sh = obs_properties_create();
	obs_property_t *shadow_alpha = add_range_slider(
		sh, kBgShadowAlpha, obs_module_text("Shadow.Opacity"),
		limits::kUnitRange);
	set_tip(shadow_alpha, obs_module_text("Tooltip.ShadowOpacity"));
	add_range_slider(
		sh, kBgShadowBlur, obs_module_text("Shadow.Blur"),
		limits::kShadowBlurRange);
	add_range_slider(
		sh, kBgShadowOffsetX, obs_module_text("Shadow.OffsetX"),
		limits::kShadowOffsetRange);
	add_range_slider(
		sh, kBgShadowOffset, obs_module_text("Shadow.Offset"),
		limits::kShadowOffsetRange);
	obs_property_t *sh_group = obs_properties_add_group(
		bg, kBgShadow, obs_module_text("Group.Shadow"), OBS_GROUP_CHECKABLE, sh);
	obs_property_set_modified_callback(sh_group, background_shadow_changed);

	obs_properties_t *frame = obs_properties_create();
	obs_property_t *fstyle = obs_properties_add_list(
		frame, kFrameStyle, obs_module_text("Frame.Style"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(fstyle, obs_module_text("Frame.Style.None"), 0);
	obs_property_list_add_int(fstyle, obs_module_text("Frame.Style.Inner"), 1);
	obs_property_list_add_int(fstyle, obs_module_text("Frame.Style.Bezel"), 2);
	obs_property_list_add_int(fstyle, obs_module_text("Frame.Style.Browser"), 3);
	obs_property_list_add_int(fstyle, obs_module_text("Frame.Style.MacBook"), 4);
	obs_property_list_add_int(fstyle, obs_module_text("Frame.Style.iPhone"), 5);
	obs_property_set_modified_callback(fstyle, frame_style_changed);
	set_tip(fstyle, obs_module_text("Tooltip.FrameStyle"));
	obs_property_t *fcolor =
		obs_properties_add_color(frame, kFrameColor, obs_module_text("Frame.Color"));
	set_tip(fcolor, obs_module_text("Tooltip.FrameColor"));
	obs_property_t *fwidth = add_range_slider(
		frame, kFrameWidth, obs_module_text("Frame.Width"),
		limits::kFrameWidthRange);
	set_tip(fwidth, obs_module_text("Tooltip.FrameWidth"));
	obs_properties_add_group(bg, "grp_frame", obs_module_text("Group.Frame"), OBS_GROUP_NORMAL, frame);
	frame_style_changed(props, nullptr, settings);

	obs_property_t *bg_group = obs_properties_add_group(
		capture, kBgEnabled, obs_module_text("Group.Background"), OBS_GROUP_CHECKABLE, bg);
	obs_property_set_modified_callback(bg_group, background_enabled_changed);
	background_enabled_changed(props, nullptr, settings);
	background_shadow_changed(props, nullptr, settings);
}

bool zoom_motion_blur_changed(obs_properties_t *props, obs_property_t *p,
			      obs_data_t *settings)
{
	(void)p;
	const bool on = settings && obs_data_get_bool(settings, kZoomMotionBlur);
	if (obs_property_t *v = obs_properties_get(props, kZoomMotionBlurAmount))
		obs_property_set_visible(v, on);
	return true;
}

bool zoom_mode_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const long long mode = settings ? obs_data_get_int(settings, kZoomMode)
					: static_cast<long long>(ZoomMode::Auto);
	const bool auto_mode = mode == static_cast<long long>(ZoomMode::Auto);
	const bool smart = mode == static_cast<long long>(ZoomMode::Smart);
	const bool active = auto_mode || smart;
	for (const char *key : {kZoomAmount, kZoomDurationMs, kZoomPanAmount,
			       kZoomCenterAmount}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, active);
	}
	if (obs_property_t *v = obs_properties_get(props, kZoomIdleTimeoutMs))
		obs_property_set_visible(v, auto_mode);
	if (obs_property_t *v = obs_properties_get(props, kZoomSmartHoldMs))
		obs_property_set_visible(v, smart);
	if (obs_property_t *v = obs_properties_get(props, kZoomActiveWindow))
		obs_property_set_visible(v, smart);
	zoom_motion_blur_changed(props, nullptr, settings);
	return true;
}

void add_zoom_section(obs_properties_t *props, obs_properties_t *capture, obs_data_t *settings)
{
	/* ---------------- Zoom ---------------- */
	obs_properties_t *zoom = obs_properties_create();
	obs_property_t *zmode = obs_properties_add_list(zoom, kZoomMode, obs_module_text("Zoom.Mode"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Off"), static_cast<int>(ZoomMode::Off));
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Auto"), static_cast<int>(ZoomMode::Auto));
	obs_property_list_add_int(zmode, obs_module_text("Zoom.Mode.Smart"), static_cast<int>(ZoomMode::Smart));
	obs_property_set_modified_callback(zmode, zoom_mode_changed);

	add_range_slider(
		zoom, kZoomAmount, obs_module_text("Zoom.Amount"),
		limits::kZoomAmountRange);
	add_range_slider(
		zoom, kZoomDurationMs, obs_module_text("Zoom.DurationMs"),
		limits::kZoomDurationRange);
	add_range_slider(
		zoom, kZoomIdleTimeoutMs, obs_module_text("Zoom.IdleTimeoutMs"),
		limits::kZoomIdleTimeoutRange);
	obs_property_t *pan = add_range_slider(
		zoom, kZoomPanAmount, obs_module_text("Zoom.PanAmount"),
		limits::kUnitRange);
	set_tip(pan, obs_module_text("Tooltip.ZoomPan"));
	obs_property_t *center = add_range_slider(
		zoom, kZoomCenterAmount, obs_module_text("Zoom.CenterAmount"),
		limits::kUnitRange);
	set_tip(center, obs_module_text("Tooltip.ZoomCentering"));
	obs_property_t *smart_hold = add_range_slider(
		zoom, kZoomSmartHoldMs, obs_module_text("Zoom.SmartHoldMs"),
		limits::kZoomSmartHoldRange);
	set_tip(smart_hold, obs_module_text("Tooltip.ZoomSmartHold"));
	obs_property_t *active_window = obs_properties_add_bool(
		zoom, kZoomActiveWindow, obs_module_text("Zoom.ActiveWindow"));
	set_tip(active_window, obs_module_text("Tooltip.ZoomActiveWindow"));
	obs_property_t *mblur = obs_properties_add_bool(
		zoom, kZoomMotionBlur, obs_module_text("Zoom.MotionBlur"));
	set_tip(mblur, obs_module_text("Tooltip.ZoomMotionBlur"));
	obs_property_set_modified_callback(mblur, zoom_motion_blur_changed);
	obs_property_t *mblur_amount = add_range_slider(
		zoom, kZoomMotionBlurAmount, obs_module_text("Zoom.MotionBlurAmount"),
		limits::kUnitRange);
	set_tip(mblur_amount, obs_module_text("Tooltip.ZoomMotionBlurAmount"));
	obs_properties_add_group(capture, "grp_zoom", obs_module_text("Group.Zoom"), OBS_GROUP_NORMAL, zoom);
	zoom_mode_changed(props, nullptr, settings);
}

} // namespace topscreen::properties
