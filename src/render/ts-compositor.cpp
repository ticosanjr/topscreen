#include "ts-compositor.hpp"

#include <graphics/vec2.h>

#include <cstdio>

#include "core/color.hpp"

namespace topscreen {

namespace {

/* Setters com guarda: um uniform ausente nao pode derrubar o OBS. */
inline void set_f(gs_eparam_t *p, float v)
{
	if (p)
		gs_effect_set_float(p, v);
}
inline void set_v2(gs_eparam_t *p, float x, float y)
{
	if (!p)
		return;
	struct vec2 v;
	vec2_set(&v, x, y);
	gs_effect_set_vec2(p, &v);
}
inline void set_c(gs_eparam_t *p, uint32_t c)
{
	if (p)
		gs_effect_set_color(p, c);
}

} // namespace

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
	validate_params();
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

	p_zoom_blur_ = P("zoom_blur");
	p_spotlight_enabled_ = P("spotlight_enabled");
	p_spotlight_radius_ = P("spotlight_radius");
	p_spotlight_dim_ = P("spotlight_dim");

	p_click_count_ = P("click_count");

	char name[32];
	for (int i = 0; i < 4; ++i) {
		std::snprintf(name, sizeof(name), "click%d_pos", i);
		click_[i].pos = P(name);
		std::snprintf(name, sizeof(name), "click%d_prog", i);
		click_[i].prog = P(name);
		std::snprintf(name, sizeof(name), "click%d_size", i);
		click_[i].size = P(name);
		std::snprintf(name, sizeof(name), "click%d_color", i);
		click_[i].color = P(name);
		std::snprintf(name, sizeof(name), "click%d_type", i);
		click_[i].type = P(name);
	}
}

void Compositor::validate_params() const
{
	if (!effect_)
		return;

	struct Named {
		const char *name;
		gs_eparam_t *param;
	};
	const Named checks[] = {
		{"target_size", p_target_size_},   {"content_pad", p_pad_},
		{"corner_radius", p_radius_},      {"zoom", p_zoom_},
		{"zoom_focus", p_focus_},          {"bg_a", p_bg_a_},
		{"bg_b", p_bg_b_},                 {"bg_mode", p_bg_mode_},
		{"cursor_enabled", p_cursor_enabled_}, {"cursor_pos", p_cursor_pos_},
		{"cursor_scale", p_cursor_scale_}, {"click_count", p_click_count_},
	};

	for (const Named &n : checks) {
		if (!n.param)
			blog(LOG_WARNING, "[TopScreen] uniform ausente no shader: %s", n.name);
	}
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
	p_zoom_blur_ = p_spotlight_enabled_ = p_spotlight_radius_ = p_spotlight_dim_ = nullptr;
	p_click_count_ = nullptr;
	for (auto &c : click_)
		c = ClickParams{};
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

	set_v2(p_target_size_, static_cast<float>(w), static_cast<float>(h));

	set_f(p_pad_, bg ? static_cast<float>(cfg.bg.padding) : 0.0f);
	set_f(p_radius_, bg ? static_cast<float>(cfg.bg.corner_radius) : 0.0f);
	set_f(p_bg_mode_, bg ? static_cast<float>(cfg.bg.mode) : 0.0f);
	set_c(p_bg_a_, cfg.bg.color_a);
	set_c(p_bg_b_, cfg.bg.color_b);

	const float shadow_alpha = (bg && cfg.bg.shadow) ? static_cast<float>(cfg.bg.shadow_alpha) : 0.0f;
	set_f(p_shadow_alpha_, shadow_alpha);
	set_f(p_shadow_blur_, static_cast<float>(cfg.bg.shadow_blur));
	set_v2(p_shadow_offset_, 0.0f, static_cast<float>(cfg.bg.shadow_blur) * 0.25f);

	float zoom = active ? static_cast<float>(st.zoom.zoom) : 1.0f;
	if (zoom < 1.0f)
		zoom = 1.0f;
	set_f(p_zoom_, zoom);
	set_v2(p_focus_, static_cast<float>(st.zoom.focus_x), static_cast<float>(st.zoom.focus_y));

	/* Cursor */
	const bool cursor_on = active && cfg.cursor.enabled && st.cursor.visible && st.cursor.opacity > 0.001;
	set_f(p_cursor_enabled_, cursor_on ? 1.0f : 0.0f);
	set_v2(p_cursor_pos_, static_cast<float>(st.cursor.x), static_cast<float>(st.cursor.y));
	set_f(p_cursor_scale_, static_cast<float>(st.cursor.size));
	set_f(p_cursor_opacity_, static_cast<float>(st.cursor.opacity));
	set_f(p_cursor_rotation_, static_cast<float>(st.cursor.rotation));
	set_f(p_cursor_shadow_, cfg.cursor.shadow ? 1.0f : 0.0f);

	/* Motion blur do zoom */
	set_f(p_zoom_blur_, active ? static_cast<float>(st.zoom_blur) : 0.0f);

	/* Spotlight */
	const bool spot = active && cfg.spotlight.enabled;
	set_f(p_spotlight_enabled_, spot ? 1.0f : 0.0f);
	set_f(p_spotlight_radius_, static_cast<float>(cfg.spotlight.radius));
	set_f(p_spotlight_dim_, static_cast<float>(cfg.spotlight.dim));

	/* Cliques (ate kShaderClickSlots, slots explicitos) */
	int used = st.click_count;
	if (used > kShaderClickSlots)
		used = kShaderClickSlots;
	set_f(p_click_count_, static_cast<float>(used));

	for (int i = 0; i < used; ++i) {
		const core::ClickRender &c = st.clicks[i];
		set_v2(click_[i].pos, static_cast<float>(c.x), static_cast<float>(c.y));
		set_f(click_[i].prog, static_cast<float>(c.progress));
		set_f(click_[i].size, static_cast<float>(c.size));
		set_c(click_[i].color, c.color);
		set_f(click_[i].type, static_cast<float>(c.effect));
	}

	obs_source_process_filter_end(filter, effect_, w, h);
}

} // namespace topscreen
