#include "cursor_atlas.hpp"

#include <cstring>

#include "json.hpp"

namespace topscreen::core {

namespace {
uint32_t rd_u32(const uint8_t *p)
{
	return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
	       (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
} // namespace

bool cursor_atlas_parse(const uint8_t *data, size_t size, CursorAtlas *out, std::string *error)
{
	auto fail = [&](const char *msg) {
		if (error)
			*error = std::string("cursor atlas: ") + msg;
		return false;
	};

	if (!data || size < 16)
		return fail("file too short");
	if (std::memcmp(data, kCursorAtlasMagic, 4) != 0)
		return fail("invalid magic");

	const uint32_t version = rd_u32(data + 4);
	if (version != kCursorAtlasVersion1 && version != kCursorAtlasVersion)
		return fail("unsupported version");

	const uint32_t cell = rd_u32(data + 8);
	const uint32_t count = rd_u32(data + 12);
	if (cell == 0 || cell > 1024 || count == 0 || count > 256)
		return fail("invalid dimensions");

	uint32_t columns = 1;
	size_t header = 16;
	if (version >= 2) {
		if (size < 20)
			return fail("file too short for the grid header");
		columns = rd_u32(data + 16);
		header = 20;
	}
	if (columns == 0 || columns > 256)
		return fail("invalid column count");

	const size_t rows = (static_cast<size_t>(count) + columns - 1) / columns;
	const size_t cells = static_cast<size_t>(columns) * rows;
	const size_t expected = header + static_cast<size_t>(cell) * cell * 4 * cells;
	if (size < expected)
		return fail("incomplete data");

	out->cell = cell;
	out->count = count;
	out->columns = columns;
	out->pixels.assign(data + header, data + expected);
	return true;
}

bool cursor_hotspots_parse(const std::string &json_text, std::vector<Vec2f> *out, std::string *error)
{
	json::Value root;
	if (!json::parse(json_text, &root, error))
		return false;

	const json::Value *cursors = root.find("cursors");
	if (!cursors || !cursors->is_array()) {
		if (error)
			*error = "cursor atlas: 'cursors' missing";
		return false;
	}

	out->clear();
	for (const json::Value &entry : *cursors->array) {
		const json::Value *hs = entry.find("hotspot");
		Vec2f v{0.05, 0.05};
		if (hs && hs->is_array() && hs->array->size() >= 2) {
			v.x = (*hs->array)[0].as_number();
			v.y = (*hs->array)[1].as_number();
		}
		/* keeps in 0..1 */
		if (v.x < 0.0)
			v.x = 0.0;
		if (v.x > 1.0)
			v.x = 1.0;
		if (v.y < 0.0)
			v.y = 0.0;
		if (v.y > 1.0)
			v.y = 1.0;
		out->push_back(v);
	}

	return true;
}

} // namespace topscreen::core
