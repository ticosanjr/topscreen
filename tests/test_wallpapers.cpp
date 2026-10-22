#include "test_framework.hpp"
#include "ts-wallpapers.hpp"

#include <string>

using namespace topscreen;

TEST(wallpapers_resolve_by_label_path_and_stem)
{
	CHECK_EQ(std::string(wallpaper_path_for("Fiber")), std::string("wallpapers/fiber.jpg"));
	CHECK_EQ(std::string(wallpaper_path_for("wallpapers/fiber.jpg")),
		 std::string("wallpapers/fiber.jpg"));
	CHECK_EQ(std::string(wallpaper_path_for("fiber")), std::string("wallpapers/fiber.jpg"));
}

TEST(wallpapers_case_insensitive)
{
	CHECK_EQ(std::string(wallpaper_path_for("FIBER")), std::string("wallpapers/fiber.jpg"));
	CHECK_EQ(std::string(wallpaper_path_for("Fiber.JPG")), std::string("wallpapers/fiber.jpg"));
	CHECK_EQ(std::string(wallpaper_path_for("HELIX")), std::string("wallpapers/helix.jpg"));
}

TEST(wallpapers_all_resolve)
{
	CHECK_EQ(kWallpaperCount, 18);
	for (int i = 0; i < kWallpaperCount; ++i) {
		const WallpaperEntry &w = kWallpapers[i];
		CHECK(wallpaper_path_for(w.name) != nullptr);
		CHECK_EQ(std::string(wallpaper_path_for(w.name)), std::string(w.path));
		CHECK_EQ(std::string(wallpaper_path_for(w.path)), std::string(w.path));
	}
}

TEST(wallpapers_removed_entries_return_nullptr)
{
	CHECK(wallpaper_path_for("Aurora") == nullptr);
	CHECK(wallpaper_path_for("Ocean") == nullptr);
	CHECK(wallpaper_path_for("Onyx") == nullptr);
	CHECK(wallpaper_path_for("wallpapers/aurora.jpg") == nullptr);
}

TEST(wallpapers_invalids_return_nullptr)
{
	CHECK(wallpaper_path_for(nullptr) == nullptr);
	CHECK(wallpaper_path_for("") == nullptr);
	CHECK(wallpaper_path_for("does-not-exist") == nullptr);
	CHECK(wallpaper_path_for("/tmp/foto.png") == nullptr);
}

TEST(wallpapers_transparent_sentinel)
{
	CHECK(wallpaper_is_transparent(kTransparentWallpaper));
	CHECK(!wallpaper_is_transparent("wallpapers/fiber.jpg"));
	CHECK(!wallpaper_is_transparent(""));
	CHECK(!wallpaper_is_transparent(nullptr));
}
