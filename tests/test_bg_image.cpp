#include "core/config.hpp"
#include "test_framework.hpp"

#include <string>

using namespace topscreen::core;

/*
 * A interface mantem "wallpaper" e "imagem do PC" mutuamente exclusivos; esta
 * funcao define quem vence se os dois vierem preenchidos (arquivo do PC).
 */
TEST(bg_resolve_path)
{
	CHECK_EQ(std::string(resolve_bg_path("wallpapers/aurora.jpg", "")),
		 std::string("wallpapers/aurora.jpg"));

	CHECK_EQ(std::string(resolve_bg_path("wallpapers/aurora.jpg", "/tmp/minha.png")),
		 std::string("/tmp/minha.png"));

	CHECK_EQ(std::string(resolve_bg_path("", "/tmp/minha.png")), std::string("/tmp/minha.png"));

	CHECK_EQ(std::string(resolve_bg_path("", "")), std::string(""));

	CHECK_EQ(std::string(resolve_bg_path(nullptr, nullptr)), std::string(""));
	CHECK_EQ(std::string(resolve_bg_path(nullptr, "/tmp/x.jpg")), std::string("/tmp/x.jpg"));
}
