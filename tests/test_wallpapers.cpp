#include "test_framework.hpp"
#include "ts-wallpapers.hpp"

#include <string>

using namespace topscreen;

TEST(wallpapers_resolve_por_rotulo_caminho_e_base)
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

TEST(wallpapers_todos_os_18_resolvem)
{
	CHECK_EQ(kWallpaperCount, 18);
	for (int i = 0; i < kWallpaperCount; ++i) {
		const WallpaperEntry &w = kWallpapers[i];
		CHECK(wallpaper_path_for(w.name) != nullptr);
		CHECK_EQ(std::string(wallpaper_path_for(w.name)), std::string(w.path));
		CHECK_EQ(std::string(wallpaper_path_for(w.path)), std::string(w.path));
	}
}

TEST(wallpapers_invalidos_retornam_nullptr)
{
	CHECK(wallpaper_path_for(nullptr) == nullptr);
	CHECK(wallpaper_path_for("") == nullptr);
	CHECK(wallpaper_path_for("nao-existe") == nullptr);
	CHECK(wallpaper_path_for("/tmp/foto.png") == nullptr);
}
