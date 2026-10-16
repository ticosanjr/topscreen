#include "ts-compositor.hpp"

#include <graphics/vec2.h>

#include "core/color.hpp"

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

	auto P = [this](const char *name) { return gs_effect_get_param_by_name(effect_, name); };

	p_target_size_ = P("target_size");
	p_pad_ = P("content_pad");
	p_radius_ = P("corner_radius");
	p_zoom_ = P("zoom");
	p_focus_ = P("zoom_focus");
	p_bg_a_ = P("bg_a");
	p_bg_b_ = P("bg_b");
	p_bg_mode_ = P("bg_mode");
	p_shadow_alpha_ = P("shadow_alpha");
	p_shadow_blur_ = P("shadow_blur");
	p_shadow_offset_ = P("shadow_offset");

	p_cursor_enabled_ = P("cursor_enabled");
	p_cursor_pos_ = P("cursor_pos");
	p_cursor_scale_ = P("cursor_scale");
	p_cursor_opacity_ = P("cursor_opacity");
	p_cursor_rotation_ = P("cursor_rotation");
	p_cursor_shadow_ = P("cursor_shadow");

	p_click_count_ = P("click_count");
	p_click_data_ = P("click_data");
	p_click_color_ = P("click_color");
	p_click_meta_ = P("click_meta");
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
	p_cursor_enabled_ = p_cursor_pos_ = p_cursor_scale_ = nullptr;
	p_cursor_opacity_ = p_cursor_rotation_ = p_cursor_shadow_ = nullptr;
	p_click_count_ = p_click_data_ = p_click_color_ = p_click_meta_ = nullptr;
}

void Compositor::render(obs_source_t *filter, const core::Config &cfg, const FrameState &st)
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

	const bool active = cfg.effects_enabled;
	const bool bg = active && cfg.bg.enabled;

	struct vec2 size;
	vec2_set(&size, static_cast<float>(w), static_cast<float>(h));
	gs_effect_set_vec2(p_target_size_, &size);

	gs_effect_set_float(p_pad_, bg ? static_cast<float>(cfg.bg.padding) : 0.0f);
	gs_effect_set_float(p_radius_, bg ? static_cast<float>(cfg.bg.corner_radius) : 0.0f);
	gs_effect_set_float(p_bg_mode_, bg ? static_cast<float>(cfg.bg.mode) : 0.0f);

	gs_effect_set_color(p_bg_a_, cfg.bg.color_a);
	gs_effect_set_color(p_bg_b_, cfg.bg.color_b);

	const float shadow_alpha = (bg && cfg.bg.shadow) ? static_cast<float>(cfg.bg.shadow_alpha) : 0.0f;
	gs_effect_set_float(p_shadow_alpha_, shadow_alpha);
	gs_effect_set_float(p_shadow_blur_, static_cast<float>(cfg.bg.shadow_blur));

	struct vec2 shadow_offset;
	vec2_set(&shadow_offset, 0.0f, static_cast<float>(cfg.bg.shadow_blur) * 0.25f);
	gs_effect_set_vec2(p_shadow_offset_, &shadow_offset);

	float zoom = active ? static_cast<float>(st.zoom.zoom) : 1.0f;
	if (zoom < 1.0f)
		zoom = 1.0f;
	gs_effect_set_float(p_zoom_, zoom);

	struct vec2 focus;
	vec2_set(&focus, static_cast<float>(st.zoom.focus_x), static_cast<float>(st.zoom.focus_y));
	gs_effect_set_vec2(p_focus_, &focus);

	/* Cursor */
	const bool cursor_on = active && cfg.cursor.enabled && st.cursor.visible && st.cursor.opacity > 0.001;
	gs_effect_set_float(p_cursor_enabled_, cursor_on ? 1.0f : 0.0f);
	struct vec2 cpos;
	vec2_set(&cpos, static_cast<float>(st.cursor.x), static_cast<float>(st.cursor.y));
	gs_effect_set_vec2(p_cursor_pos_, &cpos);
	gs_effect_set_float(p_cursor_scale_, static_cast<float>(st.cursor.size));
	gs_effect_set_float(p_cursor_opacity_, static_cast<float>(st.cursor.opacity));
	gs_effect_set_float(p_cursor_rotation_, static_cast<float>(st.cursor.rotation));
	gs_effect_set_float(p_cursor_shadow_, (cfg.cursor.shadow ? 1.0f : 0.0f));

	/* Cliques (ate kShaderClickSlots) */
	float click_data[kShaderClickSlots * 4] = {};
	float click_color[kShaderClickSlots * 4] = {};
	float click_meta[kShaderClickSlots * 4] = {};

	int used = st.click_count;
	if (used > kShaderClickSlots)
		used = kShaderClickSlots;

	for (int i = 0; i < used; ++i) {
		const core::ClickRender &c = st.clicks[i];
		click_data[i * 4 + 0] = static_cast<float>(c.x);
		click_data[i * 4 + 1] = static_cast<float>(c.y);
		click_data[i * 4 + 2] = static_cast<float>(c.progress);
		click_data[i * 4 + 3] = static_cast<float>(c.size);

		const core::Rgba col = core::unpack_abgr(c.color);
		click_color[i * 4 + 0] = col.r;
		click_color[i * 4 + 1] = col.g;
		click_color[i * 4 + 2] = col.b;
		click_color[i * 4 + 3] = col.a;

		click_meta[i * 4 + 0] = static_cast<float>(c.effect);
	}

	gs_effect_set_float(p_click_count_, static_cast<float>(used));
	gs_effect_set_val(p_click_data_, click_data, sizeof(click_data));
	gs_effect_set_val(p_click_color_, click_color, sizeof(click_color));
	gs_effect_set_val(p_click_meta_, click_meta, sizeof(click_meta));

	obs_source_process_filter_end(filter, effect_, w, h);
}

} // namespace topscreen
