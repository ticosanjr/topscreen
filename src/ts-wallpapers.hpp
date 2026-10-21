#pragma once

#include <cstring>

namespace topscreen {

struct WallpaperEntry {
	const char *name; // displayed label
	const char *path; // path relative to the module
};

/** Built-in wallpapers (data/wallpapers). */
inline constexpr WallpaperEntry kWallpapers[] = {
	{"Aurora", "wallpapers/aurora.jpg"},     {"Sunset", "wallpapers/sunset.jpg"},
	{"Monterey", "wallpapers/monterey.jpg"}, {"Ventura", "wallpapers/ventura.jpg"},
	{"Sonoma", "wallpapers/sonoma.jpg"},     {"Sequoia", "wallpapers/sequoia.jpg"},
	{"Graphite", "wallpapers/graphite.jpg"}, {"Ocean", "wallpapers/ocean.jpg"},
	{"Forest", "wallpapers/forest.jpg"},     {"Lavender", "wallpapers/lavender.jpg"},
	{"Ember", "wallpapers/ember.jpg"},       {"Midnight", "wallpapers/midnight.jpg"},
	{"Neon", "wallpapers/neon.jpg"},         {"Cyberpunk", "wallpapers/cyberpunk.jpg"},
	{"Tropical", "wallpapers/tropical.jpg"}, {"Coral", "wallpapers/coral.jpg"},
	{"Galaxy", "wallpapers/galaxy.jpg"},     {"Rainbow", "wallpapers/rainbow.jpg"},
	{"Porcelain", "wallpapers/porcelain.jpg"}, {"Sand", "wallpapers/sand.jpg"},
	{"Sage", "wallpapers/sage.jpg"},         {"Onyx", "wallpapers/onyx.jpg"},
	{"Rose", "wallpapers/rose.jpg"},         {"Azure", "wallpapers/azure.jpg"},
	/* Unsplash photography (see data/wallpapers/SOURCES.txt). */
	{"Silk", "wallpapers/silk.jpg"},         {"Pastel", "wallpapers/pastel.jpg"},
	{"Helix", "wallpapers/helix.jpg"},       {"Fiber", "wallpapers/fiber.jpg"},
	{"Prism", "wallpapers/prism.jpg"},       {"Twilight", "wallpapers/twilight.jpg"},
	{"Ink", "wallpapers/ink.jpg"},           {"Spark", "wallpapers/spark.jpg"},
	{"Cobalt", "wallpapers/cobalt.jpg"},     {"Dusk", "wallpapers/dusk.jpg"},
	{"Violet Wave", "wallpapers/violet-wave.jpg"},
	{"Wood Arch", "wallpapers/wood-arch.jpg"},
	{"Color Melt", "wallpapers/color-melt.jpg"},
	{"Ivory Dunes", "wallpapers/ivory-dunes.jpg"},
	{"Glacier", "wallpapers/glacier.jpg"},
	{"Pink Ribbon", "wallpapers/pink-ribbon.jpg"},
	{"Graphite Fold", "wallpapers/graphite-fold.jpg"},
	{"Amber Ribbon", "wallpapers/amber-ribbon.jpg"},
};

inline constexpr int kWallpaperCount = sizeof(kWallpapers) / sizeof(kWallpapers[0]);

/** Synthetic wallpaper entry: transparent outside the floating screen. */
inline constexpr const char *kTransparentWallpaper = "__transparent__";

inline bool wallpaper_is_transparent(const char *value)
{
	return value && std::strcmp(value, kTransparentWallpaper) == 0;
}

/**
 * If `value` is the label/base of a built-in wallpaper (e.g.: "Aurora" or
 * "aurora"), returns the relative path ("wallpapers/aurora.jpg").
 * Otherwise returns nullptr (treats it as a normal path).
 */
const char *wallpaper_path_for(const char *value);

} // namespace topscreen
