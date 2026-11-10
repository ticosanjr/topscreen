#pragma once

#include <obs-module.h>

namespace topscreen {

/** OBS audio source that carries click sounds into the normal mixer. */
extern struct obs_source_info ts_click_audio_source_info;

/** Adds the click audio source to the current scene (idempotent). */
bool ts_click_audio_add_to_current_scene();

} // namespace topscreen
