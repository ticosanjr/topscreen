#include "font_atlas.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <stb_truetype.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace topscreen::core {

const Glyph *FontAtlas::glyph(int code) const
{
	/* glyphs is ordered by code: binary search instead of linear scan. */
	auto it = std::lower_bound(glyphs.begin(), glyphs.end(), code,
				   [](const Glyph &g, int value) { return g.code < value; });
	if (it != glyphs.end() && it->code == code)
		return &(*it);
	return nullptr;
}

namespace {

struct Placed {
	int code = 0;
	int gx = 0, gy = 0, gw = 0, gh = 0;
	int xoff = 0, yoff = 0, adv = 0;
};

int next_pow2(int v)
{
	int p = 1;
	while (p < v)
		p <<= 1;
	return p;
}

} // namespace

bool font_atlas_build(const uint8_t *ttf, size_t ttf_size, const FontAtlasOptions &options, FontAtlas *out,
		      std::string *error)
{
	auto fail = [&](const char *msg) {
		if (error)
			*error = std::string("font: ") + msg;
		return false;
	};

	if (!ttf || ttf_size == 0)
		return fail("no font data");

	stbtt_fontinfo info;
	if (!stbtt_InitFont(&info, ttf, 0))
		return fail("invalid TTF");

	const float scale = stbtt_ScaleForPixelHeight(&info, static_cast<float>(options.pixel_height));

	int asc = 0, desc = 0, gap = 0;
	stbtt_GetFontVMetrics(&info, &asc, &desc, &gap);
	const int line_height = static_cast<int>(std::ceil((asc - desc + gap) * scale));

	const int atlas_w = 512;
	int pen_x = options.padding;
	int pen_y = options.padding;
	int row_h = 0;

	std::vector<Placed> placed;
	placed.reserve(95);

	for (int c = 32; c < 127; ++c) {
		int adv = 0, lsb = 0;
		stbtt_GetCodepointHMetrics(&info, c, &adv, &lsb);

		int ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
		stbtt_GetCodepointBitmapBox(&info, c, scale, scale, &ix0, &iy0, &ix1, &iy1);
		int gw = ix1 - ix0;
		int gh = iy1 - iy0;
		if (gw < 0)
			gw = 0;
		if (gh < 0)
			gh = 0;

		if (gw > 0 && pen_x + gw + options.padding > atlas_w) {
			pen_x = options.padding;
			pen_y += row_h + options.padding;
			row_h = 0;
		}

		Placed p;
		p.code = c;
		p.gx = pen_x;
		p.gy = pen_y;
		p.gw = gw;
		p.gh = gh;
		p.xoff = ix0;
		p.yoff = iy0;
		p.adv = static_cast<int>(std::lround(adv * scale));
		placed.push_back(p);

		pen_x += gw + options.padding;
		if (gh > row_h)
			row_h = gh;
	}

	const int needed_h = pen_y + row_h + options.padding;
	const int atlas_h = next_pow2(needed_h > 0 ? needed_h : 1);

	out->width = atlas_w;
	out->height = atlas_h;
	out->line_height = line_height;
	out->ascent = static_cast<int>(std::lround(asc * scale));
	out->pixels.assign(static_cast<size_t>(atlas_w) * atlas_h * 4u, 0);
	out->glyphs.clear();
	out->glyphs.reserve(placed.size());

	for (const Placed &p : placed) {
		if (p.gw > 0 && p.gh > 0) {
			int bw = 0, bh = 0;
			unsigned char *bmp =
				stbtt_GetCodepointBitmap(&info, scale, scale, p.code, &bw, &bh, nullptr, nullptr);
			if (bmp) {
				for (int y = 0; y < p.gh && y < bh; ++y) {
					for (int x = 0; x < p.gw && x < bw; ++x) {
						const uint8_t a = bmp[y * bw + x];
						const size_t idx = (static_cast<size_t>(p.gy + y) * atlas_w +
								    (p.gx + x)) *
								   4u;
						out->pixels[idx + 0] = 255;
						out->pixels[idx + 1] = 255;
						out->pixels[idx + 2] = 255;
						out->pixels[idx + 3] = a;
					}
				}
				stbtt_FreeBitmap(bmp, nullptr);
			}
		}

		Glyph g;
		g.code = p.code;
		g.x = p.gx;
		g.y = p.gy;
		g.w = p.gw;
		g.h = p.gh;
		g.xoff = p.xoff;
		g.yoff = p.yoff;
		g.advance = p.adv;
		out->glyphs.push_back(g);
	}

	return true;
}

} // namespace topscreen::core
