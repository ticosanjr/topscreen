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

TEST(image_decodifica_wallpaper_real)
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

TEST(image_rejeita_dados_invalidos)
{
	Image img;
	std::string err;
	CHECK(!image_decode(nullptr, 0, &img, &err));

	const std::vector<uint8_t> lixo = {1, 2, 3, 4, 5};
	CHECK(!image_decode(lixo.data(), lixo.size(), &img, &err));
	CHECK(!err.empty());
}
