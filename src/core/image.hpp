#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Refuse decompression bombs (1920x1080 = 2.1 MP, 8K = 33 MP). */
inline constexpr uint64_t kMaxImagePixels = 64ull * 1024ull * 1024ull;

inline bool image_size_allowed(int width, int height)
{
	return width > 0 && height > 0 &&
	       static_cast<uint64_t>(width) * static_cast<uint64_t>(height) <= kMaxImagePixels;
}

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
