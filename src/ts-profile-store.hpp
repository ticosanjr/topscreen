#pragma once

#include <obs-module.h>

#include <string>

#include "core/config.hpp"

namespace topscreen::profile_store {

/**
 * Makes sure all ten slots exist. A source from an older seed revision is
 * reseeded once; later calls preserve user edits. Returns true when the
 * settings object changed and needs `obs_source_update`.
 */
bool ensure_slots(obs_data_t *settings, int revision);

/** Overwrites every slot with the built-in seed and stores the revision. */
void reset_slots(obs_data_t *settings, int revision);

/** Reads one slot, using that slot's seed for absent fields. */
core::Config load_slot(obs_data_t *settings, int index);

/** Writes one slot (creating it when needed). */
void store_slot(obs_data_t *settings, int index, const core::Config &cfg);

/** Writes a single seed slot. */
void seed_slot(obs_data_t *settings, int index);

/** Custom name stored inside the slot (empty when the user did not set one). */
std::string load_name(obs_data_t *settings, int index);

/** Stores a custom name inside the slot (empty restores the A..J label). */
void store_name(obs_data_t *settings, int index, const std::string &name);

/** Menu label: the custom name when present, otherwise the A..J label. */
std::string display_name(obs_data_t *settings, int index);

} // namespace topscreen::profile_store
