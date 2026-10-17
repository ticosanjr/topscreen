#pragma once

#include "config.hpp"

namespace topscreen::core {

/**
 * Initial configuration of a profile slot (0..3).
 *
 * The slots are born with VISIBLE variations (click color, zoom and background),
 * so you can switch between profiles without having to save first.
 *
 *   A (0): default
 *   B (1): green/orange clicks + strong zoom (2.0)
 *   C (2): purple/cyan clicks + soft zoom (1.4) + vertical background
 *   D (3): yellow/white clicks + zoom off
 */
Config make_profile_seed(int index);

/**
 * Slot label (0..3): "Profile A" .. "Profile D".
 *
 * It lives in the core (and not in the locale) on purpose: the label must always
 * be distinct, even if the language file is missing/outdated.
 */
const char *profile_label(int index);

/** Normalized index (0..3) of any slot. */
int profile_index(int index);

} // namespace topscreen::core
