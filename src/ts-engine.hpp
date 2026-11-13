#pragma once

#include <graphics/graphics.h>
#include <obs-module.h>

#include <cstdint>

namespace topscreen {

/**
 * Shared TopScreen effect host.
 *
 * The filter and the capture source use the same host: settings, profiles,
 * hotkeys, input models, compositor and the per-frame state. Only the draw
 * entry point differs (the filter's built-in render vs `render_texture`). The
 * handle is opaque to the source so the filter implementation can keep its
 * private types.
 */
void *ts_engine_create(obs_data_t *settings, obs_source_t *source, bool source_mode);
void ts_engine_destroy(void *engine);
void ts_engine_update(void *engine, obs_data_t *settings);
void ts_engine_defaults(obs_data_t *settings);
void ts_engine_set_target(void *engine, uint32_t width, uint32_t height);
void ts_engine_tick(void *engine, float seconds);
void ts_engine_render_filter(void *engine, gs_effect_t *effect);
void ts_engine_render_texture(void *engine, gs_texture_t *texture, uint32_t width,
			      uint32_t height, gs_effect_t *effect);
obs_properties_t *ts_engine_properties(void *engine, obs_source_t *source);

} // namespace topscreen
