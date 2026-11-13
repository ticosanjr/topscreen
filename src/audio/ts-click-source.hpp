#pragma once

#include <obs-module.h>
#include <media-io/audio-io.h>
#include <cstddef>
#include <cstdint>

namespace topscreen {

/** OBS audio source that carries click sounds into the normal mixer. */
extern struct obs_source_info ts_click_audio_source_info;

/** Adds the click audio source to the current scene (idempotent). */
bool ts_click_audio_add_to_current_scene();

/** Adds the click audio source to every scene in the current collection. */
bool ts_click_audio_add_to_all_scenes();

/**
 * Drains the process-wide click bus into an OBS audio_render buffer.
 * Both the dedicated click source and TopScreen Capture use this helper.
 */
bool ts_click_audio_mix(uint64_t *next_ts, uint64_t *ts_out, struct obs_source_audio_mix *audio,
			uint32_t mixers, size_t channels, size_t sample_rate);

} // namespace topscreen
