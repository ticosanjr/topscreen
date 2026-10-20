#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Metrics of a glyph inside the atlas. */
struct Glyph {
	int code = 0;
	int x = 0; // position in the atlas (px)
	int y = 0;
	int w = 0; // size (px)
	int h = 0;
	int xoff = 0; // offset of the left edge relative to the advance (px)
	int yoff = 0; // offset of the top relative to the baseline (px)
	int advance = 0; // horizontal advance (px)
};

/**
 * Glyph atlas in RGBA8 (white + alpha), generated from a TTF.
 *
 * The RGB is always white; the alpha carries the coverage. This allows coloring the
 * text in the shader (e.g.: white with a dark outline).
 */
struct FontAtlas {
	int width = 0;
	int height = 0;
	int line_height = 0;
	int ascent = 0;
	std::vector<uint8_t> pixels; // RGBA8
	std::vector<Glyph> glyphs;   // ordered by `code`

	/* Direct index for ASCII: the text hot path looks up the same glyphs
	 * every frame, and a binary search per character is unnecessary. */
	std::array<int16_t, 128> ascii = [] {
		std::array<int16_t, 128> table;
		table.fill(-1);
		return table;
	}();

	const Glyph *glyph(int code) const;
};

struct FontAtlasOptions {
	int pixel_height = 32;
	int padding = 1;
};

/** Builds the atlas from the bytes of a TTF. */
bool font_atlas_build(const uint8_t *ttf, size_t ttf_size, const FontAtlasOptions &options, FontAtlas *out,
		      std::string *error);

} // namespace topscreen::core
