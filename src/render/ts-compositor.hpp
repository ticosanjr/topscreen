#pragma once

#include <graphics/graphics.h>
#include <obs-module.h>

#include "core/config.hpp"
#include "render/ts-frame.hpp"

namespace topscreen {

/**
 * Compositor: dona do efeito GPU do TopScreen.
 *
 * Recebe o frame da fonte-pai via obs_source_process_filter_* e desenha a
 * composicao (fundo, cantos, sombra, zoom/pan, cursor e efeitos de clique)
 * em um unico passe.
 */
class Compositor {
public:
	Compositor() = default;
	~Compositor();

	Compositor(const Compositor &) = delete;
	Compositor &operator=(const Compositor &) = delete;

	bool load();
	void release();

	void render(obs_source_t *filter, const core::Config &cfg, const FrameState &st);

	bool loaded() const { return effect_ != nullptr; }

private:
	void cache_params();

	gs_effect_t *effect_ = nullptr;

	gs_eparam_t *p_target_size_ = nullptr;
	gs_eparam_t *p_pad_ = nullptr;
	gs_eparam_t *p_radius_ = nullptr;
	gs_eparam_t *p_zoom_ = nullptr;
	gs_eparam_t *p_focus_ = nullptr;
	gs_eparam_t *p_bg_a_ = nullptr;
	gs_eparam_t *p_bg_b_ = nullptr;
	gs_eparam_t *p_bg_mode_ = nullptr;
	gs_eparam_t *p_shadow_alpha_ = nullptr;
	gs_eparam_t *p_shadow_blur_ = nullptr;
	gs_eparam_t *p_shadow_offset_ = nullptr;

	gs_eparam_t *p_cursor_enabled_ = nullptr;
	gs_eparam_t *p_cursor_pos_ = nullptr;
	gs_eparam_t *p_cursor_scale_ = nullptr;
	gs_eparam_t *p_cursor_opacity_ = nullptr;
	gs_eparam_t *p_cursor_rotation_ = nullptr;
	gs_eparam_t *p_cursor_shadow_ = nullptr;

	gs_eparam_t *p_click_count_ = nullptr;
	gs_eparam_t *p_click_data_ = nullptr;
	gs_eparam_t *p_click_color_ = nullptr;
	gs_eparam_t *p_click_meta_ = nullptr;
};

} // namespace topscreen
