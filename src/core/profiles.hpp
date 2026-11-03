#pragma once

#include "config.hpp"

namespace topscreen::core {

/**
 * Initial configuration of a profile slot (0..9, A..J).
 *
 * The slots are born with visible, usable variations so you can switch
 * profiles without saving first.
 *
 *   A (0): balanced everyday default
 *   B (1): clean/minimal (transparent, zoom off)
 *   C (2): presentation/teaching (keys + highlighter)
 *   D (3): streaming/gaming (solid, colorful clicks)
 *   E (4): spotlight focus
 *   F (5): magnifier lens
 *   G (6): neon energy
 *   H (7): cinematic wallpaper
 *   I (8): maximum effects
 *   J (9): full passthrough (master effects off)
 */
Config make_profile_seed(int index);

/**
 * Slot label (0..9): "Profile A" .. "Profile J".
 *
 * It lives in the core (and not in the locale) on purpose: the label must always
 * be distinct, even if the language file is missing/outdated.
 */
const char *profile_label(int index);

/** Normalized index (0..9) of any slot. */
int profile_index(int index);

} // namespace topscreen::core
