#include "core/font_atlas.hpp"
#include "test_framework.hpp"

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace topscreen::core;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

namespace {

bool load_font(std::vector<uint8_t> *out)
{
	const std::string path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/fonts/DejaVuSans-Bold.ttf";
	std::ifstream in(path, std::ios::binary);
	if (!in)
		return false;
	*out = std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	return true;
}

} // namespace

TEST(font_atlas_builds_from_ttf)
{
	std::vector<uint8_t> ttf;
	bool ok = load_font(&ttf);
	REQUIRE(ok);
	REQUIRE(!ttf.empty());

	FontAtlas atlas;
	std::string err;
	FontAtlasOptions opt;
	opt.pixel_height = 32;
	REQUIRE(font_atlas_build(ttf.data(), ttf.size(), opt, &atlas, &err));

	CHECK_EQ(atlas.width, 512);
	CHECK(atlas.height > 0);
	CHECK_EQ(atlas.height & (atlas.height - 1), 0); // power of 2
	CHECK(atlas.line_height > 0);
	CHECK(atlas.ascent > 0);
	CHECK_EQ(static_cast<int>(atlas.glyphs.size()), 95 + 14); // ASCII 32..126 + key icons
	CHECK_EQ(static_cast<int>(atlas.pixels.size()), atlas.width * atlas.height * 4);
}

TEST(font_atlas_has_key_icons)
{
	std::vector<uint8_t> ttf;
	REQUIRE(load_font(&ttf));
	FontAtlas atlas;
	std::string err;
	FontAtlasOptions opt;
	opt.pixel_height = 32;
	REQUIRE(font_atlas_build(ttf.data(), ttf.size(), opt, &atlas, &err));

	/* Special keys shown as icons: backspace, enter, arrows, modifiers. */
	static const int kIcons[] = {0x2423, 0x232B, 0x23CE, 0x21E5, 0x2190, 0x2191,
				     0x2192, 0x2193, 0x21E7, 0x2303, 0x2325, 0x229E, 0x2326,
				     0x21EA};
	for (int cp : kIcons) {
		++tst::checks();
		const Glyph *g = atlas.glyph(cp);
		if (!g)
			tst::fail(__FILE__, __LINE__, "font lacks key icon");
	}
}

TEST(font_atlas_per_glyph_metrics)
{
	std::vector<uint8_t> ttf;
	REQUIRE(load_font(&ttf));
	FontAtlas atlas;
	std::string err;
	FontAtlasOptions opt;
	opt.pixel_height = 32;
	REQUIRE(font_atlas_build(ttf.data(), ttf.size(), opt, &atlas, &err));

	const Glyph *A = atlas.glyph('A');
	REQUIRE(A != nullptr);
	CHECK(A->w > 0);
	CHECK(A->h > 0);
	CHECK(A->advance > 0);

	const Glyph *space = atlas.glyph(' ');
	REQUIRE(space != nullptr);
	CHECK_EQ(space->w, 0); // space has no bitmap
	CHECK(space->advance > 0);

	const Glyph *missing = atlas.glyph(1000);
	CHECK(missing == nullptr);
}

TEST(font_atlas_alpha_has_pixels)
{
	std::vector<uint8_t> ttf;
	REQUIRE(load_font(&ttf));
	FontAtlas atlas;
	std::string err;
	FontAtlasOptions opt;
	opt.pixel_height = 32;
	REQUIRE(font_atlas_build(ttf.data(), ttf.size(), opt, &atlas, &err));

	const Glyph *A = atlas.glyph('A');
	REQUIRE(A != nullptr);

	int set_pixels = 0;
	for (int y = 0; y < A->h; ++y) {
		for (int x = 0; x < A->w; ++x) {
			const size_t idx = (static_cast<size_t>(A->y + y) * atlas.width + (A->x + x)) * 4u;
			if (atlas.pixels[idx + 3] > 0)
				++set_pixels;
			CHECK_EQ(atlas.pixels[idx + 0], 255); // white RGB
		}
	}
	CHECK(set_pixels > 0);
}

TEST(font_atlas_ascii_lookup_matches_ordered_search)
{
	std::vector<uint8_t> ttf;
	REQUIRE(load_font(&ttf));
	FontAtlas atlas;
	std::string err;
	FontAtlasOptions opt;
	opt.pixel_height = 32;
	REQUIRE(font_atlas_build(ttf.data(), ttf.size(), opt, &atlas, &err));

	/* The fast ASCII index must return exactly the glyph the ordered
	 * search would find (including absent codes). */
	for (int code = 0; code < 128; ++code) {
		const Glyph *fast = atlas.glyph(code);
		const Glyph *slow = nullptr;
		for (const Glyph &g : atlas.glyphs) {
			if (g.code == code) {
				slow = &g;
				break;
			}
		}
		CHECK_EQ(fast, slow);
	}

	/* Symbols still use the binary search and keep working. */
	const Glyph *icon = atlas.glyph(0x2423);
	REQUIRE(icon != nullptr);
	CHECK_EQ(icon->code, 0x2423);
	CHECK(atlas.glyph(0x10FFFF) == nullptr);

	/* An atlas that was never built must not resolve ASCII codes. */
	FontAtlas empty;
	CHECK(empty.glyph('A') == nullptr);
	CHECK(empty.glyph(' ') == nullptr);
}

TEST(font_atlas_rejects_invalid_data)
{
	FontAtlas atlas;
	std::string err;
	FontAtlasOptions opt;
	CHECK(!font_atlas_build(nullptr, 0, opt, &atlas, &err));

	const std::vector<uint8_t> junk = {1, 2, 3, 4, 5, 6, 7, 8};
	CHECK(!font_atlas_build(junk.data(), junk.size(), opt, &atlas, &err));
	CHECK(!err.empty());

	/* Large zeroed buffer: table count is zero, stb must bail out cleanly. */
	const std::vector<uint8_t> zeros(1024 * 1024, 0);
	CHECK(!font_atlas_build(zeros.data(), zeros.size(), opt, &atlas, &err));

	/* Truncated TTC collection claiming many fonts. */
	const std::vector<uint8_t> ttc = {'t', 't', 'c', 'f', 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF};
	CHECK(!font_atlas_build(ttc.data(), ttc.size(), opt, &atlas, &err));
}
