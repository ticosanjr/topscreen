#pragma once

#include <obs-module.h>

/**
 * TopScreen - filter source (OBS_SOURCE_TYPE_FILTER).
 *
 * The filter processes the parent source's frame (screen capture) applying the effects
 * live and returns the result to the scene. No data leaves the OBS process.
 */
extern struct obs_source_info ts_filter_source_info;
