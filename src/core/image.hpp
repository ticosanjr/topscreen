#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Decoded image (RGBA8, top->bottom). */
struct Image {
	int width = 0;
	int height = 0;
	std::vector<uint8_t> pixels;

	bool empty() const { return width <= 0 || height <= 0 || pixels.empty(); }
};

/** Decodes PNG/JPG/BMP/etc. (stb_image) from an in-memory buffer. */
bool image_decode(const uint8_t *data, size_t size, Image *out, std::string *error);

} // namespace topscreen::core
