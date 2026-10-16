#include "ts-compositor.hpp"

#include <graphics/vec2.h>

namespace topscreen {

Compositor::~Compositor()
{
	release();
}

bool Compositor::load()
{
	if (effect_)
		return true;

	char *path = obs_module_file("effects/topscreen.effect");
	if (!path) {
		blog(LOG_ERROR, "[TopScreen] obs_module_file(effects/topscreen.effect) falhou");
		return false;
	}

	obs_enter_graphics();
	effect_ = gs_effect_create_from_file(path, nullptr);
	obs_leave_graphics();

	bfree(path);

	if (!effect_) {
		blog(LOG_ERROR, "[TopScreen] falha ao criar o efeito topscreen.effect");
		return false;
	}

	cache_params();
	return true;
}

void Compositor::cache_params()
{
	if (!effect_)
		return;

	p_target_size_ = gs_effect_get_param_by_name(effect_, "target_size");
	p_pad_ = gs_effect_get_param_by_name(effect_, "content_pad");
	p_radius_ = gs_effect_get_param_by_name(effect_, "corner_radius");
	p_zoom_ = gs_effect_get_param_by_name(effect_, "zoom");
	p_focus_ = gs_effect_get_param_by_name(effect_, "zoom_focus");
	p_bg_a_ = gs_effect_get_param_by_name(effect_, "bg_a");
	p_bg_b_ = gs_effect_get_param_by_name(effect_, "bg_b");
	p_bg_mode_ = gs_effect_get_param_by_name(effect_, "bg_mode");
	p_shadow_alpha_ = gs_effect_get_param_by_name(effect_, "shadow_alpha");
	p_shadow_blur_ = gs_effect_get_param_by_name(effect_, "shadow_blur");
	p_shadow_offset_ = gs_effect_get_param_by_name(effect_, "shadow_offset");
}

void Compositor::release()
{
	if (!effect_)
		return;

	obs_enter_graphics();
	gs_effect_destroy(effect_);
	obs_leave_graphics();

	effect_ = nullptr;
	p_target_size_ = p_pad_ = p_radius_ = p_zoom_ = p_focus_ = nullptr;
	p_bg_a_ = p_bg_b_ = p_bg_mode_ = nullptr;
	p_shadow_alpha_ = p_shadow_blur_ = p_shadow_offset_ = nullptr;
}

void Compositor::render(obs_source_t *filter, const Config &cfg)
{
	obs_source_t *target = obs_filter_get_target(filter);
	if (!target) {
		obs_source_skip_video_filter(filter);
		return;
	}

	const uint32_t w = obs_source_get_base_width(target);
	const uint32_t h = obs_source_get_base_height(target);
	if (!w || !h) {
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!effect_) {
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!obs_source_process_filter_begin(filter, GS_RGBA, OBS_NO_DIRECT_RENDERING))
		return;

	/* Todo o conjunto de efeitos respeita o master toggle. */
	const bool active = cfg.effects_enabled;
	const bool bg = active && cfg.bg.enabled;

	struct vec2 size;
	vec2_set(&size, static_cast<float>(w), static_cast<float>(h));
	gs_effect_set_vec2(p_target_size_, &size);

	gs_effect_set_float(p_pad_, bg ? static_cast<float>(cfg.bg.padding) : 0.0f);
	gs_effect_set_float(p_radius_, bg ? static_cast<float>(cfg.bg.corner_radius) : 0.0f);
	gs_effect_set_float(p_bg_mode_, bg ? static_cast<float>(cfg.bg.mode) : 0.0f);

	gs_effect_set_color(p_bg_a_, static_cast<uint32_t>(cfg.bg.color_a));
	gs_effect_set_color(p_bg_b_, static_cast<uint32_t>(cfg.bg.color_b));

	const float shadow_alpha = (bg && cfg.bg.shadow) ? static_cast<float>(cfg.bg.shadow_alpha) : 0.0f;
	gs_effect_set_float(p_shadow_alpha_, shadow_alpha);
	gs_effect_set_float(p_shadow_blur_, static_cast<float>(cfg.bg.shadow_blur));

	struct vec2 shadow_offset;
	vec2_set(&shadow_offset, 0.0f, static_cast<float>(cfg.bg.shadow_blur) * 0.25f);
	gs_effect_set_vec2(p_shadow_offset_, &shadow_offset);

	float zoom = active ? static_cast<float>(cfg.zoom.zoom) : 1.0f;
	if (zoom < 1.0f)
		zoom = 1.0f;
	gs_effect_set_float(p_zoom_, zoom);

	struct vec2 focus;
	vec2_set(&focus, static_cast<float>(cfg.zoom.focus_x), static_cast<float>(cfg.zoom.focus_y));
	gs_effect_set_vec2(p_focus_, &focus);

	obs_source_process_filter_end(filter, effect_, w, h);
}

} // namespace topscreen
