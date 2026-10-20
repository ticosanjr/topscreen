#include "core/image.hpp"
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
std::vector<uint8_t> read_bytes(const std::string &path)
{
	std::ifstream in(path, std::ios::binary);
	return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}
} // namespace

TEST(image_decodes_real_wallpaper)
{
	const auto bytes = read_bytes(std::string(TOPSCREEN_TEST_DATA_DIR) + "/wallpapers/ocean.jpg");
	REQUIRE(!bytes.empty());

	Image img;
	std::string err;
	REQUIRE(image_decode(bytes.data(), bytes.size(), &img, &err));

	CHECK_EQ(img.width, 1920);
	CHECK_EQ(img.height, 1080);
	CHECK_EQ(static_cast<int>(img.pixels.size()), img.width * img.height * 4);
	CHECK(!img.empty());
}

TEST(image_rejects_invalid_data)
{
	Image img;
	std::string err;
	CHECK(!image_decode(nullptr, 0, &img, &err));

	const std::vector<uint8_t> lixo = {1, 2, 3, 4, 5};
	CHECK(!image_decode(lixo.data(), lixo.size(), &img, &err));
	CHECK(!err.empty());
}

/* A tiny file can declare huge dimensions (decompression bomb). The header
 * check must reject it before the pixel buffer is allocated. */
TEST(image_rejects_oversized_dimensions)
{
	CHECK(image_size_allowed(1920, 1080));
	CHECK(image_size_allowed(7680, 4320));
	CHECK(!image_size_allowed(30000, 30000));
	CHECK(!image_size_allowed(0, 1080));
	CHECK(!image_size_allowed(-1, 1080));

	/* 9000x9000 = 81 MP > the 64 MP ceiling. */
	CHECK(!image_size_allowed(9000, 9000));
}
