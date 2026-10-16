#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Metrica de um glifo dentro do atlas. */
struct Glyph {
	int code = 0;
	int x = 0; // posicao no atlas (px)
	int y = 0;
	int w = 0; // tamanho (px)
	int h = 0;
	int xoff = 0; // deslocamento do canto esquerdo em relacao ao avanco (px)
	int yoff = 0; // deslocamento do topo em relacao a baseline (px)
	int advance = 0; // avanco horizontal (px)
};

/**
 * Atlas de glifos em RGBA8 (branco + alfa), gerado a partir de um TTF.
 *
 * O RGB e sempre branco; o alfa carrega a cobertura. Isso permite colorir o
 * texto no shader (ex.: branco com contorno escuro).
 */
struct FontAtlas {
	int width = 0;
	int height = 0;
	int line_height = 0;
	int ascent = 0;
	std::vector<uint8_t> pixels; // RGBA8
	std::vector<Glyph> glyphs;   // ordenado por `code`

	const Glyph *glyph(int code) const;
};

struct FontAtlasOptions {
	int pixel_height = 32;
	int padding = 1;
};

/** Constroi o atlas a partir dos bytes de um TTF. */
bool font_atlas_build(const uint8_t *ttf, size_t ttf_size, const FontAtlasOptions &options, FontAtlas *out,
		      std::string *error);

} // namespace topscreen::core
