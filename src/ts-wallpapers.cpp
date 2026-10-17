#include "ts-wallpapers.hpp"

#include <cctype>
#include <cstring>
#include <string>

namespace topscreen {

namespace {

bool iequals(const char *a, const char *b)
{
	if (!a || !b)
		return false;
	while (*a && *b) {
		if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b)))
			return false;
		++a;
		++b;
	}
	return *a == '\0' && *b == '\0';
}

/* "wallpapers/aurora.jpg" -> "aurora" */
std::string stem_of(const char *path)
{
	const char *base = std::strrchr(path, '/');
	base = base ? base + 1 : path;
	std::string s(base);
	const auto dot = s.find_last_of('.');
	if (dot != std::string::npos)
		s = s.substr(0, dot);
	return s;
}

} // namespace

const char *wallpaper_path_for(const char *value)
{
	if (!value || !*value)
		return nullptr;

	/* Stems of the built-in paths, computed only once. */
	static std::string stems[kWallpaperCount];
	static bool stems_init = false;
	if (!stems_init) {
		for (int i = 0; i < kWallpaperCount; ++i)
			stems[i] = stem_of(kWallpapers[i].path);
		stems_init = true;
	}

	const std::string vs = stem_of(value);
	for (int i = 0; i < kWallpaperCount; ++i) {
		const WallpaperEntry &w = kWallpapers[i];
		if (iequals(value, w.name) || iequals(value, w.path))
			return w.path;
		if (iequals(vs.c_str(), stems[i].c_str()))
			return w.path;
	}
	return nullptr;
}

} // namespace topscreen
