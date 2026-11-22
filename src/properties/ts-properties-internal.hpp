#pragma once

#include <obs-module.h>

#include "core/config_limits.hpp"

namespace topscreen::properties {

/* Shared helpers (ts-properties-common.cpp). */
void set_tip(obs_property_t *prop, const char *description);

obs_property_t *add_range_slider(obs_properties_t *props, const char *key, const char *text,
				 const core::limits::Range &range);

obs_property_t *add_range_int_slider(obs_properties_t *props, const char *key, const char *text,
				     const core::limits::IntRange &range);

obs_property_t *add_range_int(obs_properties_t *props, const char *key, const char *text,
			      const core::limits::IntRange &range);

bool setting_bool(obs_data_t *settings, const char *key, bool fallback);
long long setting_int(obs_data_t *settings, const char *key, long long fallback);

/* Section builders, grouped by the top-level Properties tabs. */
void add_background_section(obs_properties_t *props, obs_properties_t *capture,
			    obs_data_t *settings);
void add_zoom_section(obs_properties_t *props, obs_properties_t *capture, obs_data_t *settings);

void add_cursor_section(obs_properties_t *props, obs_properties_t *pointer, obs_data_t *settings);
void add_clicks_section(obs_properties_t *props, obs_properties_t *pointer, obs_data_t *settings);
void add_click_sound_section(obs_properties_t *props, obs_properties_t *pointer,
			     obs_data_t *settings, void *data);

void add_spotlight_section(obs_properties_t *props, obs_properties_t *focus,
			   obs_data_t *settings);
void add_magnifier_section(obs_properties_t *props, obs_properties_t *focus,
			   obs_data_t *settings);

void add_highlight_section(obs_properties_t *props, obs_properties_t *overlays,
			   obs_data_t *settings);
void add_keys_section(obs_properties_t *props, obs_properties_t *overlays, obs_data_t *settings);

} // namespace topscreen::properties
