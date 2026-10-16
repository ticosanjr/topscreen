#include "ts-sdf-font.hpp"

#include <obs-module.h>

namespace topscreen {

bool SdfFont::load(const char *path)
{
	/* TODO(T5.1): carregar a fonte e construir o atlas SDF. */
	UNUSED_PARAMETER(path);
	loaded_ = false;
	return false;
}

void SdfFont::release()
{
	loaded_ = false;
	atlas_ = nullptr;
}

} // namespace topscreen
