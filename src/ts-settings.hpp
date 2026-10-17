#pragma once

#include <obs-module.h>

#include "core/config.hpp"

namespace topscreen {

/** Fills in the defaults in obs_data (used in get_defaults). */
void config_defaults(obs_data_t *settings);

/** Reads the configuration from obs_data. */
void config_load(obs_data_t *settings, core::Config *out);

/** Writes the configuration to obs_data (used by presets/hotkeys). */
void config_store(obs_data_t *settings, const core::Config &cfg);

} // namespace topscreen
