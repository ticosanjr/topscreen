#pragma once

#include <obs-module.h>

namespace topscreen::parent_cursor {

/** Registers this filter as a user of the parent's native cursor. */
void acquire(obs_source_t *parent);

/** Adds/removes one "hide native cursor" want from this filter. */
void set_wanted(obs_source_t *parent, bool hide);

/** Releases the lease and restores the cursor when the last user leaves. */
void release(obs_source_t *parent, bool was_hiding);

} // namespace topscreen::parent_cursor
