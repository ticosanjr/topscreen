#pragma once

#include <graphics/graphics.h>
#include <obs-module.h>

#include "ts-settings.hpp"

namespace topscreen {

/**
 * Compositor: dona do efeito GPU do TopScreen.
 *
 * Recebe o frame da fonte-pai via obs_source_process_filter_* e desenha a
 * composicao (fundo, cantos, sombra, zoom/pan) em um unico passe.
 */
class Compositor {
public:
	Compositor() = default;
	~Compositor();

	Compositor(const Compositor &) = delete;
	Compositor &operator=(const Compositor &) = delete;

	/** Carrega o efeito. Requer contexto grafico (tratado internamente). */
	bool load();

	/** Libera o efeito. Requer contexto grafico (tratado internamente). */
	void release();

	/**
	 * Renderiza o filtro aplicando a config.
	 * Deve ser chamado dentro de video_render (contexto grafico ativo).
	 */
	void render(obs_source_t *filter, const Config &cfg);

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
};

} // namespace topscreen
