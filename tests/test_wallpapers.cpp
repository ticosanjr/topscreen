#include "test_framework.hpp"
#include "ts-wallpapers.hpp"

#include <string>

using namespace topscreen;

TEST(wallpapers_resolve_by_label_path_and_stem)
{
	CHECK_EQ(std::string(wallpaper_path_for("Aurora")), std::string("wallpapers/aurora.jpg"));
	CHECK_EQ(std::string(wallpaper_path_for("wallpapers/aurora.jpg")),
		 std::string("wallpapers/aurora.jpg"));
	CHECK_EQ(std::string(wallpaper_path_for("aurora")), std::string("wallpapers/aurora.jpg"));
}

TEST(wallpapers_case_insensitive)
{
	CHECK_EQ(std::string(wallpaper_path_for("AURORA")), std::string("wallpapers/aurora.jpg"));
	CHECK_EQ(std::string(wallpaper_path_for("Aurora.JPG")), std::string("wallpapers/aurora.jpg"));
	CHECK_EQ(std::string(wallpaper_path_for("MONTEREY")), std::string("wallpapers/monterey.jpg"));
}

TEST(wallpapers_all_resolve)
{
	CHECK_EQ(kWallpaperCount, 42);
	for (int i = 0; i < kWallpaperCount; ++i) {
		const WallpaperEntry &w = kWallpapers[i];
		CHECK(wallpaper_path_for(w.name) != nullptr);
		CHECK_EQ(std::string(wallpaper_path_for(w.name)), std::string(w.path));
		CHECK_EQ(std::string(wallpaper_path_for(w.path)), std::string(w.path));
	}
}

TEST(wallpapers_invalids_return_nullptr)
{
	CHECK(wallpaper_path_for(nullptr) == nullptr);
	CHECK(wallpaper_path_for("") == nullptr);
	CHECK(wallpaper_path_for("does-not-exist") == nullptr);
	CHECK(wallpaper_path_for("/tmp/foto.png") == nullptr);
}
