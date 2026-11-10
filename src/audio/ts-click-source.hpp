#pragma once

#include <obs-module.h>

namespace topscreen {

/** OBS audio source that carries click sounds into the normal mixer. */
extern struct obs_source_info ts_click_audio_source_info;

/** Adds the click audio source to the current scene (idempotent). */
bool ts_click_audio_add_to_current_scene();

/** Adds the click audio source to every scene in the current collection. */
bool ts_click_audio_add_to_all_scenes();

} // namespace topscreen
