#pragma once

#include <obs-module.h>

#include "core/config.hpp"

namespace topscreen {

/** Fills in the defaults in obs_data (used in get_defaults). */
void config_defaults(obs_data_t *settings);

/** Reads the configuration from obs_data. When fallback is non-null, fields
 *  absent from the data use it instead of the global defaults (used to fill
 *  partial profile backups with that slot's seed). */
void config_load(obs_data_t *settings, core::Config *out,
		 const core::Config *fallback = nullptr);

/** Writes the configuration to obs_data (used by presets/hotkeys). */
void config_store(obs_data_t *settings, const core::Config &cfg);

/** Writes only the fields that differ from `base` (sparse profile slots). */
void config_store_sparse(obs_data_t *settings, const core::Config &cfg,
			 const core::Config &base);

} // namespace topscreen
