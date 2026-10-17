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
	CHECK_EQ(static_cast<int>(atlas.glyphs.size()), 95); // ASCII 32..126
	CHECK_EQ(static_cast<int>(atlas.pixels.size()), atlas.width * atlas.height * 4);
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

TEST(font_atlas_rejects_invalid_data)
{
	FontAtlas atlas;
	std::string err;
	FontAtlasOptions opt;
	CHECK(!font_atlas_build(nullptr, 0, opt, &atlas, &err));

	const std::vector<uint8_t> lixo = {1, 2, 3, 4, 5, 6, 7, 8};
	CHECK(!font_atlas_build(lixo.data(), lixo.size(), opt, &atlas, &err));
	CHECK(!err.empty());
}
