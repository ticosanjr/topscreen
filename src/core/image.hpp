#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Imagem decodificada (RGBA8, topo->baixo). */
struct Image {
	int width = 0;
	int height = 0;
	std::vector<uint8_t> pixels;

	bool empty() const { return width <= 0 || height <= 0 || pixels.empty(); }
};

/** Decodifica PNG/JPG/BMP/etc. (stb_image) de um buffer em memoria. */
bool image_decode(const uint8_t *data, size_t size, Image *out, std::string *error);

} // namespace topscreen::core
