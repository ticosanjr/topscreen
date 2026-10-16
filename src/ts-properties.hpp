#pragma once

#include <obs-module.h>

namespace topscreen {

/** Cria a arvore de propriedades nativa do OBS para o filtro. */
obs_properties_t *properties_create(void *data);

} // namespace topscreen
