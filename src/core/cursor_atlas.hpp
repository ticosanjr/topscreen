#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "types.hpp"

namespace topscreen::core {

inline constexpr uint32_t kCursorAtlasVersion = 1;
inline constexpr char kCursorAtlasMagic[4] = {'T', 'S', 'A', 'T'};

/** Atlas de cursores: `count` celulas de `cell` x `cell` (RGBA8, empilhadas). */
struct CursorAtlas {
	uint32_t cell = 0;
	uint32_t count = 0;
	std::vector<uint8_t> pixels;

	uint32_t width() const { return cell; }
	uint32_t height() const { return cell * count; }
	bool empty() const { return count == 0 || pixels.empty(); }
};

/** Le o formato TSAT (magic/version/cell/count + pixels RGBA8). */
bool cursor_atlas_parse(const uint8_t *data, size_t size, CursorAtlas *out, std::string *error);

/** Le os hotspots (0..1) do atlas.json, na mesma ordem das celulas. */
bool cursor_hotspots_parse(const std::string &json_text, std::vector<Vec2f> *out, std::string *error);

} // namespace topscreen::core
