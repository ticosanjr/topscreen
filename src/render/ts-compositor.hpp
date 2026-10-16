#pragma once

#include <graphics/graphics.h>
#include <obs-module.h>

#include "core/config.hpp"
#include "render/ts-frame.hpp"
#include "render/ts-text.hpp"

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
	void validate_params() const;

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

	gs_eparam_t *p_zoom_blur_ = nullptr;
	gs_eparam_t *p_spotlight_enabled_ = nullptr;
	gs_eparam_t *p_spotlight_radius_ = nullptr;
	gs_eparam_t *p_spotlight_dim_ = nullptr;

	gs_eparam_t *p_highlight_active_ = nullptr;
	gs_eparam_t *p_highlight_rect_ = nullptr;
	gs_eparam_t *p_highlight_color_ = nullptr;
	gs_eparam_t *p_highlight_alpha_ = nullptr;

	/* Slots de clique explicitos (o shader nao usa arrays de uniform). */
	struct ClickParams {
		gs_eparam_t *pos = nullptr;
		gs_eparam_t *prog = nullptr;
		gs_eparam_t *size = nullptr;
		gs_eparam_t *color = nullptr;
		gs_eparam_t *type = nullptr;
	};

	gs_eparam_t *p_click_count_ = nullptr;
	ClickParams click_[4];

	TextRenderer text_;
};

} // namespace topscreen
