#pragma once

#include <obs-module.h>

namespace topscreen {

/**
 * Creates the OBS native properties tree for the filter.
 * `settings` (optional, may be null) is only read to pick the initial
 * visibility of the mode-dependent highlighter groups.
 */
obs_properties_t *properties_create(void *data, obs_data_t *settings = nullptr);

} // namespace topscreen
