#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "types.hpp"

namespace topscreen::core {

inline constexpr uint32_t kCursorAtlasVersion1 = 1;
inline constexpr uint32_t kCursorAtlasVersion = 2;
inline constexpr char kCursorAtlasMagic[4] = {'T', 'S', 'A', 'T'};

/** Cursor atlas: `count` cells of `cell` x `cell` in a row-major grid. */
struct CursorAtlas {
	uint32_t cell = 0;
	uint32_t count = 0;
	uint32_t columns = 1;
	std::vector<uint8_t> pixels;

	uint32_t rows() const { return columns ? (count + columns - 1) / columns : 0; }
	uint32_t width() const { return cell * columns; }
	uint32_t height() const { return cell * rows(); }
	bool empty() const { return count == 0 || pixels.empty(); }

	/** Copies one cell (row-major RGBA) into `dst` (cell*cell*4 bytes). */
	bool copy_cell(uint32_t index, uint8_t *dst) const
	{
		if (index >= count || columns == 0 || pixels.empty() || !dst)
			return false;
		const uint32_t row = index / columns;
		const uint32_t col = index % columns;
		const size_t stride = static_cast<size_t>(width()) * 4;
		const size_t start = (static_cast<size_t>(row) * cell * width() +
				      static_cast<size_t>(col) * cell) *
				     4;
		for (uint32_t y = 0; y < cell; ++y)
			std::memcpy(dst + static_cast<size_t>(y) * cell * 4,
				    pixels.data() + start + static_cast<size_t>(y) * stride,
				    static_cast<size_t>(cell) * 4);
		return true;
	}
};

/** Reads the TSAT format (magic/version/cell/count + RGBA8 pixels). */
bool cursor_atlas_parse(const uint8_t *data, size_t size, CursorAtlas *out, std::string *error);

/** Reads the hotspots (0..1) from atlas.json, in the same order as the cells. */
bool cursor_hotspots_parse(const std::string &json_text, std::vector<Vec2f> *out, std::string *error);

} // namespace topscreen::core
