#include "image.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <stb_image.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace topscreen::core {

bool image_decode(const uint8_t *data, size_t size, Image *out, std::string *error)
{
	auto fail = [&](const char *msg) {
		if (error)
			*error = std::string("imagem: ") + msg;
		return false;
	};

	if (!data || size == 0)
		return fail("sem dados");

	int w = 0, h = 0, comp = 0;
	stbi_uc *pixels = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &comp, 4);
	if (!pixels)
		return fail("formato nao suportado");

	const size_t total = static_cast<size_t>(w) * h * 4u;
	out->width = w;
	out->height = h;
	out->pixels.assign(pixels, pixels + total);
	stbi_image_free(pixels);
	return true;
}

// Garante que a implementacao (definida em stb_image_impl.cpp) esta linkada.
void image_link_anchor(void);

} // namespace topscreen::core
