#include "properties/ts-properties-internal.hpp"

#include "core/config.hpp"
#include "core/config_limits.hpp"
#include "core/cursor_catalog.hpp"
#include "core/keys.hpp"
#include "core/types.hpp"

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen::properties {

bool spotlight_options_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kSpotlightEnabled, false);
	const long long shape =
		setting_int(settings, kSpotlightShape, static_cast<long long>(SpotlightShape::Circle));
	const bool blur = setting_int(settings, kSpotlightOutsideMode, 0) != 0;
	for (const char *key : {kSpotlightShowCursor, kSpotlightShape, kSpotlightRadius,
			       kSpotlightCornerRadius, kSpotlightOutsideMode, kSpotlightDim,
			       kSpotlightBlurAmount, "grp_spotlight_border",
			       "grp_spotlight_animation"}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	if (obs_property_t *v = obs_properties_get(props, kSpotlightCornerRadius))
		obs_property_set_visible(
			v, on && shape == static_cast<long long>(SpotlightShape::Square));
	if (obs_property_t *v = obs_properties_get(props, kSpotlightDim))
		obs_property_set_visible(v, on && !blur);
	if (obs_property_t *v = obs_properties_get(props, kSpotlightBlurAmount))
		obs_property_set_visible(v, on && blur);
	if (setting_bool(settings, kSpotlightEnabled, false) &&
	    setting_bool(settings, kMagnifierEnabled, false))
		obs_data_set_bool(settings, kMagnifierEnabled, false);
	return true;
}

bool spotlight_border_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kSpotlightBorderEnabled, true);
	for (const char *key : {kSpotlightBorderColor, kSpotlightBorderWidth}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	return true;
}

bool magnifier_options_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kMagnifierEnabled, false);
	const long long shape =
		setting_int(settings, kMagnifierShape, static_cast<long long>(SpotlightShape::Circle));
	const bool follow = setting_bool(settings, kMagnifierFollowCursor, true);
	for (const char *key : {kMagnifierShowCursor, kMagnifierShape, kMagnifierRadius,
			       kMagnifierZoom, kMagnifierCornerRadius,
			       kMagnifierFollowCursor, kMagnifierCenterX, kMagnifierCenterY,
			       "grp_magnifier_border", "grp_magnifier_shadow", kMagnifierInMs,
			       kMagnifierOutMs}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	if (obs_property_t *v = obs_properties_get(props, kMagnifierCornerRadius))
		obs_property_set_visible(
			v, on && shape == static_cast<long long>(SpotlightShape::Square));
	if (obs_property_t *v = obs_properties_get(props, kMagnifierCenterX))
		obs_property_set_visible(v, on && !follow);
	if (obs_property_t *v = obs_properties_get(props, kMagnifierCenterY))
		obs_property_set_visible(v, on && !follow);
	if (setting_bool(settings, kMagnifierEnabled, false) &&
	    setting_bool(settings, kSpotlightEnabled, false))
		obs_data_set_bool(settings, kSpotlightEnabled, false);
	return true;
}

bool magnifier_border_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kMagnifierBorderEnabled, true);
	for (const char *key : {kMagnifierBorderColor, kMagnifierBorderWidth}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	return true;
}

bool magnifier_shadow_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	(void)p;
	const bool on = setting_bool(settings, kMagnifierShadowEnabled, false);
	for (const char *key : {kMagnifierShadowColor, kMagnifierShadowAlpha,
			       kMagnifierShadowBlur, kMagnifierShadowOffsetX,
			       kMagnifierShadowOffsetY}) {
		if (obs_property_t *v = obs_properties_get(props, key))
			obs_property_set_visible(v, on);
	}
	return true;
}

void add_spotlight_section(obs_properties_t *props, obs_properties_t *focus, obs_data_t *settings)
{
	/* ---------------- Spotlight ---------------- */
	obs_properties_t *spot = obs_properties_create();
	obs_properties_add_bool(spot, kSpotlightShowCursor, obs_module_text("Spotlight.ShowCursor"));

	obs_property_t *sshape = obs_properties_add_list(
		spot, kSpotlightShape, obs_module_text("Spotlight.Shape"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(sshape, obs_module_text("Spotlight.Shape.Circle"),
				  static_cast<int>(core::SpotlightShape::Circle));
	obs_property_list_add_int(sshape, obs_module_text("Spotlight.Shape.Square"),
				  static_cast<int>(core::SpotlightShape::Square));
	obs_property_set_modified_callback(sshape, spotlight_options_changed);

	add_range_slider(
		spot, kSpotlightRadius, obs_module_text("Spotlight.Radius"),
		limits::kRadiusRange);
	add_range_slider(
		spot, kSpotlightCornerRadius, obs_module_text("Spotlight.CornerRadius"),
		limits::kCornerRadiusRange);
	obs_property_t *omode = obs_properties_add_list(
		spot, kSpotlightOutsideMode, obs_module_text("Spotlight.OutsideMode"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(omode, obs_module_text("Spotlight.Outside.Dim"), 0);
	obs_property_list_add_int(omode, obs_module_text("Spotlight.Outside.Blur"), 1);
	obs_property_set_modified_callback(omode, spotlight_options_changed);
	obs_property_t *dim = add_range_slider(
		spot, kSpotlightDim, obs_module_text("Spotlight.Dim"),
		limits::kUnitRange);
	set_tip(dim, obs_module_text("Tooltip.SpotlightDim"));
	add_range_slider(
		spot, kSpotlightBlurAmount, obs_module_text("Spotlight.BlurAmount"),
		limits::kBgImageBlurRange);

	obs_properties_t *sborder = obs_properties_create();
	obs_property_t *spot_border_on = obs_properties_add_bool(
		sborder, kSpotlightBorderEnabled, obs_module_text("Spotlight.BorderEnabled"));
	obs_property_set_modified_callback(spot_border_on, spotlight_border_changed);
	obs_properties_add_color(sborder, kSpotlightBorderColor,
				 obs_module_text("Spotlight.BorderColor"));
	add_range_slider(
		sborder, kSpotlightBorderWidth, obs_module_text("Spotlight.BorderWidth"),
		limits::kBorderWidthRange);
	obs_properties_add_group(spot, "grp_spotlight_border", obs_module_text("Spotlight.BorderGroup"),
				 OBS_GROUP_NORMAL, sborder);

	obs_properties_t *sanim = obs_properties_create();
	add_range_slider(
		sanim, kSpotlightZoomAmount, obs_module_text("Spotlight.ZoomAmount"),
		limits::kZoomAmountRange);
	add_range_slider(
		sanim, kSpotlightInMs, obs_module_text("Spotlight.InMs"),
		limits::kInOutMsRange);
	add_range_slider(
		sanim, kSpotlightOutMs, obs_module_text("Spotlight.OutMs"),
		limits::kInOutMsRange);
	obs_properties_add_group(spot, "grp_spotlight_animation",
				 obs_module_text("Spotlight.AnimationGroup"), OBS_GROUP_NORMAL,
				 sanim);

	obs_property_t *spot_group = obs_properties_add_group(
		focus, kSpotlightEnabled, obs_module_text("Group.Spotlight"),
		OBS_GROUP_CHECKABLE, spot);
	obs_property_set_modified_callback(spot_group, spotlight_options_changed);
	spotlight_options_changed(props, nullptr, settings);
	spotlight_border_changed(props, nullptr, settings);
}

void add_magnifier_section(obs_properties_t *props, obs_properties_t *focus, obs_data_t *settings)
{
	/* ---------------- Magnifier ---------------- */
	obs_properties_t *mag = obs_properties_create();
	obs_properties_add_bool(mag, kMagnifierShowCursor, obs_module_text("Magnifier.ShowCursor"));
	obs_property_t *mshape = obs_properties_add_list(
		mag, kMagnifierShape, obs_module_text("Magnifier.Shape"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mshape, obs_module_text("Magnifier.Shape.Circle"),
				  static_cast<int>(SpotlightShape::Circle));
	obs_property_list_add_int(mshape, obs_module_text("Magnifier.Shape.Square"),
				  static_cast<int>(SpotlightShape::Square));
	obs_property_set_modified_callback(mshape, magnifier_options_changed);
	add_range_slider(
		mag, kMagnifierRadius, obs_module_text("Magnifier.Radius"),
		limits::kRadiusRange);
	add_range_slider(
		mag, kMagnifierZoom, obs_module_text("Magnifier.Zoom"),
		limits::kZoomAmountRange);
	add_range_slider(
		mag, kMagnifierCornerRadius, obs_module_text("Magnifier.CornerRadius"),
		limits::kCornerRadiusRange);
	obs_property_t *magnifier_follow =
		obs_properties_add_bool(mag, kMagnifierFollowCursor, obs_module_text("Magnifier.FollowCursor"));
	obs_property_set_modified_callback(magnifier_follow, magnifier_options_changed);
	add_range_slider(
		mag, kMagnifierCenterX, obs_module_text("Magnifier.CenterX"),
		limits::kUnitRange);
	add_range_slider(
		mag, kMagnifierCenterY, obs_module_text("Magnifier.CenterY"),
		limits::kUnitRange);
	obs_properties_t *mborder = obs_properties_create();
	obs_property_t *mag_border_on = obs_properties_add_bool(
		mborder, kMagnifierBorderEnabled, obs_module_text("Magnifier.BorderEnabled"));
	obs_property_set_modified_callback(mag_border_on, magnifier_border_changed);
	obs_properties_add_color(mborder, kMagnifierBorderColor,
				 obs_module_text("Magnifier.BorderColor"));
	add_range_slider(
		mborder, kMagnifierBorderWidth, obs_module_text("Magnifier.BorderWidth"),
		limits::kBorderWidthRange);
	obs_properties_add_group(mag, "grp_magnifier_border", obs_module_text("Magnifier.BorderGroup"),
				 OBS_GROUP_NORMAL, mborder);
	obs_properties_t *mshadow = obs_properties_create();
	obs_property_t *mag_shadow_on = obs_properties_add_bool(
		mshadow, kMagnifierShadowEnabled, obs_module_text("Magnifier.ShadowEnabled"));
	obs_property_set_modified_callback(mag_shadow_on, magnifier_shadow_changed);
	obs_properties_add_color(mshadow, kMagnifierShadowColor,
				 obs_module_text("Magnifier.ShadowColor"));
	add_range_slider(
		mshadow, kMagnifierShadowAlpha, obs_module_text("Magnifier.ShadowAlpha"),
		limits::kUnitRange);
	add_range_slider(
		mshadow, kMagnifierShadowBlur, obs_module_text("Magnifier.ShadowBlur"),
		limits::kShadowBlurRange);
	add_range_slider(
		mshadow, kMagnifierShadowOffsetX, obs_module_text("Magnifier.ShadowOffsetX"),
		limits::kShadowOffsetRange);
	add_range_slider(
		mshadow, kMagnifierShadowOffsetY, obs_module_text("Magnifier.ShadowOffsetY"),
		limits::kShadowOffsetRange);
	obs_properties_add_group(mag, "grp_magnifier_shadow", obs_module_text("Magnifier.ShadowGroup"),
				 OBS_GROUP_NORMAL, mshadow);
	add_range_slider(
		mag, kMagnifierInMs, obs_module_text("Magnifier.InMs"),
		limits::kInOutMsRange);
	add_range_slider(
		mag, kMagnifierOutMs, obs_module_text("Magnifier.OutMs"),
		limits::kInOutMsRange);
	obs_property_t *mag_group = obs_properties_add_group(
		focus, kMagnifierEnabled, obs_module_text("Group.Magnifier"),
		OBS_GROUP_CHECKABLE, mag);
	obs_property_set_modified_callback(mag_group, magnifier_options_changed);
	magnifier_options_changed(props, nullptr, settings);
	magnifier_border_changed(props, nullptr, settings);
	magnifier_shadow_changed(props, nullptr, settings);
}

} // namespace topscreen::properties
