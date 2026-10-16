#pragma once

namespace topscreen {

struct WallpaperEntry {
	const char *name; // rotulo exibido
	const char *path; // caminho relativo ao modulo
};

/** Wallpapers embutidos (data/wallpapers). */
inline constexpr WallpaperEntry kWallpapers[] = {
	{"Aurora", "wallpapers/aurora.jpg"},     {"Sunset", "wallpapers/sunset.jpg"},
	{"Monterey", "wallpapers/monterey.jpg"}, {"Ventura", "wallpapers/ventura.jpg"},
	{"Sonoma", "wallpapers/sonoma.jpg"},     {"Sequoia", "wallpapers/sequoia.jpg"},
	{"Graphite", "wallpapers/graphite.jpg"}, {"Ocean", "wallpapers/ocean.jpg"},
	{"Forest", "wallpapers/forest.jpg"},     {"Lavender", "wallpapers/lavender.jpg"},
	{"Ember", "wallpapers/ember.jpg"},       {"Midnight", "wallpapers/midnight.jpg"},
};

inline constexpr int kWallpaperCount = sizeof(kWallpapers) / sizeof(kWallpapers[0]);

/**
 * Se `value` for o rotulo/base de um wallpaper embutido (ex.: "Aurora" ou
 * "aurora"), devolve o caminho relativo ("wallpapers/aurora.jpg").
 * Caso contrario devolve nullptr (trata como caminho normal).
 */
const char *wallpaper_path_for(const char *value);

} // namespace topscreen
