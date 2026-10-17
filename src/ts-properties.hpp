#pragma once

#include <obs-module.h>

namespace topscreen {

/** Creates the OBS native properties tree for the filter. */
obs_properties_t *properties_create(void *data);

} // namespace topscreen
