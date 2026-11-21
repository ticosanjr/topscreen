#pragma once

#include <obs-module.h>

namespace topscreen {

/**
 * Creates the OBS native properties tree for the filter.
 * `settings` (optional, may be null) is only read to pick the initial
 * visibility of the mode-dependent highlighter groups.
 */
using properties_prefix_fn = void (*)(obs_properties_t *props, void *data);

/**
 * `prefix` (optional) adds content before the filter's own groups; the capture
 * source uses it to put the monitor/source controls first.
 */
obs_properties_t *properties_create(void *data, obs_data_t *settings = nullptr,
				    bool include_monitor = true,
				    properties_prefix_fn prefix = nullptr,
				    void *prefix_data = nullptr);

} // namespace topscreen
