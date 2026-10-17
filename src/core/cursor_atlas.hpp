#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "types.hpp"

namespace topscreen::core {

inline constexpr uint32_t kCursorAtlasVersion = 1;
inline constexpr char kCursorAtlasMagic[4] = {'T', 'S', 'A', 'T'};

/** Cursor atlas: `count` cells of `cell` x `cell` (RGBA8, stacked). */
struct CursorAtlas {
	uint32_t cell = 0;
	uint32_t count = 0;
	std::vector<uint8_t> pixels;

	uint32_t width() const { return cell; }
	uint32_t height() const { return cell * count; }
	bool empty() const { return count == 0 || pixels.empty(); }
};

/** Reads the TSAT format (magic/version/cell/count + RGBA8 pixels). */
bool cursor_atlas_parse(const uint8_t *data, size_t size, CursorAtlas *out, std::string *error);

/** Reads the hotspots (0..1) from atlas.json, in the same order as the cells. */
bool cursor_hotspots_parse(const std::string &json_text, std::vector<Vec2f> *out, std::string *error);

} // namespace topscreen::core
