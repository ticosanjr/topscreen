#include "ts-webcam-filter.hpp"

#include "core/config_limits.hpp"
#include "core/keys.hpp"
#include "core/webcam.hpp"

#include <graphics/graphics.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace topscreen::core;
using namespace topscreen::core::keys;
using namespace topscreen::core::limits;

namespace topscreen {

namespace {

void set_f(gs_eparam_t *p, float v)
{
	if (p)
		gs_effect_set_float(p, v);
}

void set_v2(gs_eparam_t *p, float x, float y)
{
	if (p) {
		struct vec2 v;
		vec2_set(&v, x, y);
		gs_effect_set_vec2(p, &v);
	}
}

void set_c(gs_eparam_t *p, uint32_t abgr)
{
	if (p)
		gs_effect_set_color(p, abgr);
}

WebcamConfig load_config(obs_data_t *settings)
{
	WebcamConfig cfg;
	if (!settings)
		return cfg;
	cfg.shape = static_cast<int>(obs_data_get_int(settings, kWebcamShape));
	cfg.size = obs_data_get_double(settings, kWebcamSize);
	cfg.anchor = static_cast<int>(obs_data_get_int(settings, kWebcamAnchor));
	cfg.offset_x = obs_data_get_double(settings, kWebcamOffsetX);
	cfg.offset_y = obs_data_get_double(settings, kWebcamOffsetY);
	cfg.corner_ratio = obs_data_get_double(settings, kWebcamCorner);
	cfg.mirror = obs_data_get_bool(settings, kWebcamMirror);
	cfg.border_enabled = obs_data_get_bool(settings, kWebcamBorderEnabled);
	cfg.border_color = static_cast<uint32_t>(obs_data_get_int(settings, kWebcamBorderColor));
	cfg.border_width = obs_data_get_double(settings, kWebcamBorderWidth);
	cfg.shadow_enabled = obs_data_get_bool(settings, kWebcamShadowEnabled);
	cfg.shadow_color = static_cast<uint32_t>(obs_data_get_int(settings, kWebcamShadowColor));
	cfg.shadow_alpha = obs_data_get_double(settings, kWebcamShadowAlpha);
	cfg.shadow_blur = obs_data_get_double(settings, kWebcamShadowBlur);
	cfg.shadow_offset_x = obs_data_get_double(settings, kWebcamShadowOffsetX);
	cfg.shadow_offset_y = obs_data_get_double(settings, kWebcamShadowOffsetY);
	sanitize_webcam_config(&cfg);
	return cfg;
}

struct WebcamFilter {
	obs_source_t *source = nullptr;
	WebcamConfig cfg;
	gs_effect_t *effect = nullptr;
	bool ready = false;
	bool tried_load = false;
	bool logged_render = false;

	gs_eparam_t *p_target = nullptr;
	gs_eparam_t *p_center = nullptr;
	gs_eparam_t *p_half = nullptr;
	gs_eparam_t *p_corner = nullptr;
	gs_eparam_t *p_shape = nullptr;
	gs_eparam_t *p_mirror = nullptr;
	gs_eparam_t *p_border_enabled = nullptr;
	gs_eparam_t *p_border_color = nullptr;
	gs_eparam_t *p_border_width = nullptr;
	gs_eparam_t *p_shadow_enabled = nullptr;
	gs_eparam_t *p_shadow_color = nullptr;
	gs_eparam_t *p_shadow_alpha = nullptr;
	gs_eparam_t *p_shadow_blur = nullptr;
	gs_eparam_t *p_shadow_offset = nullptr;

	bool load_effect();
	void release();
	void render(obs_source_t *filter, obs_source_t *target, uint32_t w, uint32_t h);
};

bool WebcamFilter::load_effect()
{
	char *path = obs_module_file("effects/webcam.effect");
	if (!path)
		return false;

	char *error = nullptr;
	effect = gs_effect_create_from_file(path, &error);
	bfree(path);
	if (error)
		bfree(error);
	if (!effect)
		return false;

	auto P = [this](const char *name) { return gs_effect_get_param_by_name(effect, name); };
	p_target = P("target_size");
	p_center = P("bubble_center");
	p_half = P("bubble_half");
	p_corner = P("corner_radius");
	p_shape = P("shape");
	p_mirror = P("mirror");
	p_border_enabled = P("border_enabled");
	p_border_color = P("border_color");
	p_border_width = P("border_width");
	p_shadow_enabled = P("shadow_enabled");
	p_shadow_color = P("shadow_color");
	p_shadow_alpha = P("shadow_alpha");
	p_shadow_blur = P("shadow_blur");
	p_shadow_offset = P("shadow_offset");

	ready = p_target && p_center && p_half && p_corner && p_shape && p_mirror &&
		p_border_enabled && p_border_color && p_border_width && p_shadow_enabled &&
		p_shadow_color && p_shadow_alpha && p_shadow_blur && p_shadow_offset &&
		gs_effect_get_technique(effect, "Draw") != nullptr;
	if (!ready)
		blog(LOG_WARNING, "[TopScreen] webcam bubble effect is incomplete");
	else
		blog(LOG_INFO, "[TopScreen] webcam bubble effect loaded");
	return ready;
}

void WebcamFilter::release()
{
	if (effect) {
		obs_enter_graphics();
		gs_effect_destroy(effect);
		obs_leave_graphics();
	}
	effect = nullptr;
	ready = false;
}

void WebcamFilter::render(obs_source_t *filter, obs_source_t *target, uint32_t w, uint32_t h)
{
	if (!effect && !tried_load) {
		tried_load = true;
		load_effect();
	}
	if (!target || !w || !h || !ready) {
		obs_source_skip_video_filter(filter);
		return;
	}
	if (!obs_source_process_filter_begin(filter, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
		obs_source_skip_video_filter(filter);
		return;
	}

	const double width = static_cast<double>(w);
	const double height = static_cast<double>(h);
	const double diameter = webcam_bubble_diameter(cfg, width, height);
	const Vec2f center = webcam_bubble_center(cfg, width, height);

	set_v2(p_target, static_cast<float>(w), static_cast<float>(h));
	set_v2(p_center, static_cast<float>(center.x), static_cast<float>(center.y));
	set_v2(p_half, static_cast<float>(diameter * 0.5), static_cast<float>(diameter * 0.5));
	set_f(p_corner, static_cast<float>(cfg.corner_ratio * diameter));
	set_f(p_shape, static_cast<float>(cfg.shape));
	set_f(p_mirror, cfg.mirror ? 1.0f : 0.0f);
	set_f(p_border_enabled, cfg.border_enabled ? 1.0f : 0.0f);
	set_c(p_border_color, cfg.border_color);
	set_f(p_border_width, static_cast<float>(cfg.border_width));
	set_f(p_shadow_enabled, cfg.shadow_enabled ? 1.0f : 0.0f);
	set_c(p_shadow_color, cfg.shadow_color);
	set_f(p_shadow_alpha, static_cast<float>(cfg.shadow_alpha));
	set_f(p_shadow_blur, static_cast<float>(cfg.shadow_blur));
	set_v2(p_shadow_offset, static_cast<float>(cfg.shadow_offset_x),
	       static_cast<float>(cfg.shadow_offset_y));

	obs_source_process_filter_tech_end(filter, effect, w, h, "Draw");
}

const char *webcam_name(void *)
{
	return obs_module_text("Webcam.Name");
}

void *webcam_create(obs_data_t *settings, obs_source_t *source)
{
	auto *ctx = new WebcamFilter();
	ctx->source = source;
	ctx->cfg = load_config(settings);
	return ctx;
}

void webcam_destroy(void *data)
{
	auto *ctx = static_cast<WebcamFilter *>(data);
	if (!ctx)
		return;
	ctx->release();
	delete ctx;
}

void webcam_update(void *data, obs_data_t *settings)
{
	auto *ctx = static_cast<WebcamFilter *>(data);
	if (ctx)
		ctx->cfg = load_config(settings);
}

void webcam_tick(void *, float) {}

void webcam_render(void *data, gs_effect_t *)
{
	auto *ctx = static_cast<WebcamFilter *>(data);
	if (!ctx)
		return;
	if (!ctx->logged_render) {
		ctx->logged_render = true;
		blog(LOG_INFO, "[TopScreen] webcam bubble filter first render");
	}
	obs_source_t *target = obs_filter_get_target(ctx->source);
	uint32_t w = target ? obs_source_get_base_width(target) : 0;
	uint32_t h = target ? obs_source_get_base_height(target) : 0;
	ctx->render(ctx->source, target, w, h);
}

void webcam_defaults(obs_data_t *settings)
{
	const WebcamConfig cfg;
	obs_data_set_default_int(settings, kWebcamShape, cfg.shape);
	obs_data_set_default_double(settings, kWebcamSize, cfg.size);
	obs_data_set_default_int(settings, kWebcamAnchor, cfg.anchor);
	obs_data_set_default_double(settings, kWebcamOffsetX, cfg.offset_x);
	obs_data_set_default_double(settings, kWebcamOffsetY, cfg.offset_y);
	obs_data_set_default_double(settings, kWebcamCorner, cfg.corner_ratio);
	obs_data_set_default_bool(settings, kWebcamMirror, cfg.mirror);
	obs_data_set_default_bool(settings, kWebcamBorderEnabled, cfg.border_enabled);
	obs_data_set_default_int(settings, kWebcamBorderColor, cfg.border_color);
	obs_data_set_default_double(settings, kWebcamBorderWidth, cfg.border_width);
	obs_data_set_default_bool(settings, kWebcamShadowEnabled, cfg.shadow_enabled);
	obs_data_set_default_int(settings, kWebcamShadowColor, cfg.shadow_color);
	obs_data_set_default_double(settings, kWebcamShadowAlpha, cfg.shadow_alpha);
	obs_data_set_default_double(settings, kWebcamShadowBlur, cfg.shadow_blur);
	obs_data_set_default_double(settings, kWebcamShadowOffsetX, cfg.shadow_offset_x);
	obs_data_set_default_double(settings, kWebcamShadowOffsetY, cfg.shadow_offset_y);
}

void set_tip(obs_property_t *prop, const char *text)
{
	if (prop)
		obs_property_set_long_description(prop, text);
}

bool border_changed(obs_properties_t *props, obs_property_t *, obs_data_t *settings)
{
	const bool on = !settings || obs_data_get_bool(settings, kWebcamBorderEnabled);
	for (const char *key : {kWebcamBorderColor, kWebcamBorderWidth}) {
		if (obs_property_t *p = obs_properties_get(props, key))
			obs_property_set_visible(p, on);
	}
	return true;
}

bool shadow_changed(obs_properties_t *props, obs_property_t *, obs_data_t *settings)
{
	const bool on = !settings || obs_data_get_bool(settings, kWebcamShadowEnabled);
	for (const char *key : {kWebcamShadowColor, kWebcamShadowAlpha, kWebcamShadowBlur,
			       kWebcamShadowOffsetX, kWebcamShadowOffsetY}) {
		if (obs_property_t *p = obs_properties_get(props, key))
			obs_property_set_visible(p, on);
	}
	return true;
}

bool shape_changed(obs_properties_t *props, obs_property_t *, obs_data_t *settings)
{
	const bool rounded = !settings ||
		obs_data_get_int(settings, kWebcamShape) == static_cast<long long>(WebcamShape::Rounded);
	if (obs_property_t *p = obs_properties_get(props, kWebcamCorner))
		obs_property_set_visible(p, rounded);
	return true;
}

namespace {

void add_webcam_mask_group(obs_properties_t *props)
{
	obs_properties_t *mask = obs_properties_create();
	obs_property_t *shape = obs_properties_add_list(
		mask, kWebcamShape, obs_module_text("Webcam.Shape"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(shape, obs_module_text("Webcam.Shape.Circle"),
				  static_cast<int>(WebcamShape::Circle));
	obs_property_list_add_int(shape, obs_module_text("Webcam.Shape.Rounded"),
				  static_cast<int>(WebcamShape::Rounded));
	obs_property_set_modified_callback(shape, shape_changed);

	obs_properties_add_float_slider(mask, kWebcamSize, obs_module_text("Webcam.Size"),
					limits::kWebcamSizeRange.min, limits::kWebcamSizeRange.max,
					limits::kWebcamSizeRange.step);
	obs_properties_add_float_slider(mask, kWebcamCorner, obs_module_text("Webcam.Corner"),
					limits::kWebcamCornerRange.min,
					limits::kWebcamCornerRange.max,
					limits::kWebcamCornerRange.step);
	obs_properties_add_bool(mask, kWebcamMirror, obs_module_text("Webcam.Mirror"));

	obs_property_t *anchor = obs_properties_add_list(
		mask, kWebcamAnchor, obs_module_text("Webcam.Anchor"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.TopLeft"),
				  static_cast<int>(WebcamAnchor::TopLeft));
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.TopCenter"),
				  static_cast<int>(WebcamAnchor::TopCenter));
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.TopRight"),
				  static_cast<int>(WebcamAnchor::TopRight));
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.MiddleLeft"),
				  static_cast<int>(WebcamAnchor::MiddleLeft));
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.MiddleCenter"),
				  static_cast<int>(WebcamAnchor::MiddleCenter));
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.MiddleRight"),
				  static_cast<int>(WebcamAnchor::MiddleRight));
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.BottomLeft"),
				  static_cast<int>(WebcamAnchor::BottomLeft));
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.BottomCenter"),
				  static_cast<int>(WebcamAnchor::BottomCenter));
	obs_property_list_add_int(anchor, obs_module_text("Webcam.Anchor.BottomRight"),
				  static_cast<int>(WebcamAnchor::BottomRight));

	obs_properties_add_float_slider(mask, kWebcamOffsetX, obs_module_text("Webcam.OffsetX"),
					limits::kWebcamOffsetRange.min,
					limits::kWebcamOffsetRange.max,
					limits::kWebcamOffsetRange.step);
	obs_properties_add_float_slider(mask, kWebcamOffsetY, obs_module_text("Webcam.OffsetY"),
					limits::kWebcamOffsetRange.min,
					limits::kWebcamOffsetRange.max,
					limits::kWebcamOffsetRange.step);
	obs_properties_add_group(props, "webcam_mask", obs_module_text("Webcam.MaskGroup"),
				 OBS_GROUP_NORMAL, mask);
}

void add_webcam_border_group(obs_properties_t *props)
{
	obs_properties_t *border = obs_properties_create();
	obs_property_t *border_on = obs_properties_add_bool(
		border, kWebcamBorderEnabled, obs_module_text("Webcam.BorderEnabled"));
	obs_property_set_modified_callback(border_on, border_changed);
	obs_properties_add_color(border, kWebcamBorderColor, obs_module_text("Webcam.BorderColor"));
	obs_properties_add_float_slider(border, kWebcamBorderWidth,
					obs_module_text("Webcam.BorderWidth"),
					limits::kWebcamBorderWidthRange.min,
					limits::kWebcamBorderWidthRange.max,
					limits::kWebcamBorderWidthRange.step);
	obs_properties_add_group(props, "webcam_border", obs_module_text("Webcam.BorderGroup"),
				 OBS_GROUP_NORMAL, border);
}

void add_webcam_shadow_group(obs_properties_t *props)
{
	obs_properties_t *shadow = obs_properties_create();
	obs_property_t *shadow_on = obs_properties_add_bool(
		shadow, kWebcamShadowEnabled, obs_module_text("Webcam.ShadowEnabled"));
	obs_property_set_modified_callback(shadow_on, shadow_changed);
	obs_properties_add_color(shadow, kWebcamShadowColor, obs_module_text("Webcam.ShadowColor"));
	obs_properties_add_float_slider(shadow, kWebcamShadowAlpha,
					obs_module_text("Webcam.ShadowAlpha"),
					limits::kUnitRange.min, limits::kUnitRange.max,
					limits::kUnitRange.step);
	obs_properties_add_float_slider(shadow, kWebcamShadowBlur,
					obs_module_text("Webcam.ShadowBlur"),
					limits::kWebcamShadowBlurRange.min,
					limits::kWebcamShadowBlurRange.max,
					limits::kWebcamShadowBlurRange.step);
	obs_properties_add_float_slider(shadow, kWebcamShadowOffsetX,
					obs_module_text("Webcam.ShadowOffsetX"),
					limits::kWebcamShadowOffsetRange.min,
					limits::kWebcamShadowOffsetRange.max,
					limits::kWebcamShadowOffsetRange.step);
	obs_properties_add_float_slider(shadow, kWebcamShadowOffsetY,
					obs_module_text("Webcam.ShadowOffsetY"),
					limits::kWebcamShadowOffsetRange.min,
					limits::kWebcamShadowOffsetRange.max,
					limits::kWebcamShadowOffsetRange.step);
	obs_properties_add_group(props, "webcam_shadow", obs_module_text("Webcam.ShadowGroup"),
				 OBS_GROUP_NORMAL, shadow);
}
} // namespace

obs_properties_t *webcam_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "webcam_info", obs_module_text("Webcam.Info"), OBS_TEXT_INFO);
	add_webcam_mask_group(props);
	add_webcam_border_group(props);
	add_webcam_shadow_group(props);

	set_tip(obs_properties_get(props, kWebcamSize), obs_module_text("Webcam.Tip.Size"));
	set_tip(obs_properties_get(props, kWebcamAnchor), obs_module_text("Webcam.Tip.Anchor"));
	set_tip(obs_properties_get(props, kWebcamBorderWidth),
		obs_module_text("Webcam.Tip.BorderWidth"));
	set_tip(obs_properties_get(props, kWebcamShadowBlur),
		obs_module_text("Webcam.Tip.ShadowBlur"));

	obs_data_t *settings = nullptr;
	if (auto *ctx = static_cast<WebcamFilter *>(data))
		if (ctx->source)
			settings = obs_source_get_settings(ctx->source);
	shape_changed(props, nullptr, settings);
	border_changed(props, nullptr, settings);
	shadow_changed(props, nullptr, settings);
	obs_data_release(settings);
	return props;
}

} // namespace

struct obs_source_info ts_webcam_filter_source_info = {
	.id = "topscreen_webcam_bubble",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = webcam_name,
	.create = webcam_create,
	.destroy = webcam_destroy,
	.get_defaults = webcam_defaults,
	.get_properties = webcam_properties,
	.update = webcam_update,
	.video_tick = webcam_tick,
	.video_render = webcam_render,
};

} // namespace topscreen
