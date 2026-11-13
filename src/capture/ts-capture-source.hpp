#pragma once

#include <obs-module.h>

namespace topscreen {

/** Native screen capture source (currently X11/XShm). */
extern struct obs_source_info ts_capture_source_info;

/**
 * Runtime cursor override used by the filter's parent-cursor lease.
 *
 * Unlike the `capture_cursor` source setting this is never persisted, so a
 * scene saved while the TopScreen cursor is active cannot leave the source
 * without its native cursor after a restart.
 */
void set_native_cursor(obs_source_t *source, bool enabled);
bool native_cursor_enabled(obs_source_t *source);

} // namespace topscreen
