#include "core/cursor_atlas.hpp"
#include "test_framework.hpp"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using namespace topscreen::core;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

namespace {

std::vector<uint8_t> read_bytes(const std::string &path)
{
	std::ifstream in(path, std::ios::binary);
	return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::string read_text(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

std::string cursors_dir()
{
	return std::string(TOPSCREEN_TEST_DATA_DIR) + "/cursors";
}

} // namespace

TEST(cursor_atlas_real_file)
{
	const auto bytes = read_bytes(cursors_dir() + "/atlas.rgba");
	REQUIRE(!bytes.empty());

	CursorAtlas atlas;
	std::string err;
	REQUIRE(cursor_atlas_parse(bytes.data(), bytes.size(), &atlas, &err));

	CHECK(atlas.cell == 128);
	CHECK(atlas.count > 0);
	/* one style per CursorStyle */
	CHECK_EQ(static_cast<int>(atlas.count), static_cast<int>(CursorStyle::Count));
	CHECK(atlas.height() == atlas.cell * atlas.count);
	CHECK_EQ(static_cast<int>(atlas.pixels.size()),
		 static_cast<int>(atlas.cell * atlas.cell * atlas.count * 4));
}

TEST(cursor_atlas_hotspots_json)
{
	const std::string json_text = read_text(cursors_dir() + "/atlas.json");
	REQUIRE(!json_text.empty());

	std::vector<Vec2f> hotspots;
	std::string err;
	REQUIRE(cursor_hotspots_parse(json_text, &hotspots, &err));
	REQUIRE(!hotspots.empty());

	CHECK_EQ(static_cast<int>(hotspots.size()), static_cast<int>(CursorStyle::Count));

	for (const Vec2f &h : hotspots) {
		CHECK(h.x >= 0.0 && h.x <= 1.0);
		CHECK(h.y >= 0.0 && h.y <= 1.0);
	}
}

TEST(cursor_atlas_rejects_invalid_data)
{
	CursorAtlas atlas;
	std::string err;
	CHECK(!cursor_atlas_parse(nullptr, 0, &atlas, &err));

	const std::vector<uint8_t> curto = {1, 2, 3};
	CHECK(!cursor_atlas_parse(curto.data(), curto.size(), &atlas, &err));

	/* wrong magic */
	std::vector<uint8_t> ruim(64, 0);
	ruim[0] = 'X';
	CHECK(!cursor_atlas_parse(ruim.data(), ruim.size(), &atlas, &err));

	/* header ok but data missing */
	std::vector<uint8_t> truncado(16, 0);
	truncado[0] = 'T';
	truncado[1] = 'S';
	truncado[2] = 'A';
	truncado[3] = 'T';
	truncado[4] = 1;              /* version */
	truncado[8] = 128;            /* cell */
	truncado[12] = 2;             /* count (no pixels) */
	CHECK(!cursor_atlas_parse(truncado.data(), truncado.size(), &atlas, &err));
	CHECK(!err.empty());
}
