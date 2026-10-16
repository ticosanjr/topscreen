#pragma once

#include <obs-module.h>

#include "core/config.hpp"

namespace topscreen {

/** Preenche os defaults no obs_data (usado em get_defaults). */
void config_defaults(obs_data_t *settings);

/** Le a configuracao a partir do obs_data. */
void config_load(obs_data_t *settings, core::Config *out);

/** Escreve a configuracao no obs_data (usado por presets/atalhos). */
void config_store(obs_data_t *settings, const core::Config &cfg);

} // namespace topscreen
