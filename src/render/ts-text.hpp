#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <graphics/graphics.h>

#include "core/font_atlas.hpp"
#include "core/key_overlay.hpp"

namespace topscreen {

/**
 * Renderer de texto (overlay de teclas).
 *
 * Gera o atlas a partir de um TTF embutido, cria a textura e desenha os glifos
 * como sprites, com contorno escuro para ficar legivel em qualquer fundo.
 */
class TextRenderer {
public:
	TextRenderer() = default;
	~TextRenderer();

	TextRenderer(const TextRenderer &) = delete;
	TextRenderer &operator=(const TextRenderer &) = delete;

	/** Carrega o TTF e o efeito (nomes relativos ao modulo). */
	bool load(const char *ttf_module_file, const char *effect_module_file);
	void release();

	/** Desenha as teclas, centralizadas na base do alvo. */
	void draw(const core::KeyRender *items, int count, uint32_t target_w, uint32_t target_h);

	bool loaded() const { return texture_ != nullptr && effect_ != nullptr; }

private:
	float measure(const std::string &text, float scale) const;
	void draw_string(const std::string &text, float x, float baseline, float scale);
	void draw_labels(const std::vector<std::string> &labels, float x0, float baseline, float scale, float gap);

	core::FontAtlas atlas_;
	gs_texture_t *texture_ = nullptr;
	gs_effect_t *effect_ = nullptr;
	gs_eparam_t *p_image_ = nullptr;
	gs_eparam_t *p_color_ = nullptr;
};

} // namespace topscreen
