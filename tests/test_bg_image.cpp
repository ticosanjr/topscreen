#include "core/config.hpp"
#include "test_framework.hpp"

#include <string>

using namespace topscreen::core;

/*
 * The UI keeps "wallpaper" and "PC image" mutually exclusive; this
 * function defines which one wins if both come filled in (the PC file).
 */
TEST(bg_resolve_path)
{
	CHECK_EQ(std::string(resolve_bg_path("wallpapers/silk.jpg", "")),
		 std::string("wallpapers/silk.jpg"));

	CHECK_EQ(std::string(resolve_bg_path("wallpapers/silk.jpg", "/tmp/my-image.png")),
		 std::string("/tmp/my-image.png"));

	CHECK_EQ(std::string(resolve_bg_path("", "/tmp/my-image.png")), std::string("/tmp/my-image.png"));

	CHECK_EQ(std::string(resolve_bg_path("", "")), std::string(""));

	CHECK_EQ(std::string(resolve_bg_path(nullptr, nullptr)), std::string(""));
	CHECK_EQ(std::string(resolve_bg_path(nullptr, "/tmp/x.jpg")), std::string("/tmp/x.jpg"));
}
