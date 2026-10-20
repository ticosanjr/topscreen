#pragma once

#include <cstddef>
#include <string>

namespace topscreen::core {

/** Sanity cap for a profiles backup (the real file is a few KB). */
inline constexpr std::size_t kMaxProfileBundleBytes = 4u * 1024u * 1024u;

/** Metadata read from a TopScreen profiles backup. */
struct ProfileBundleInfo {
	int current = 0;
	int slot_count = 0;
};

/**
 * Validates a profiles backup file (the JSON produced by "Export profiles").
 *
 * The bundle must carry the marker and all 10 slots; unknown keys are ignored,
 * so a newer TopScreen can add fields without breaking older imports.
 */
bool profile_bundle_validate(const std::string &json_text, ProfileBundleInfo *out,
			     std::string *error);

} // namespace topscreen::core
