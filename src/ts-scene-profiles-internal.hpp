#pragma once

#include <util/config-file.h>

#include <map>
#include <string>

namespace topscreen::scene_profiles::detail {

/** Legacy name-keyed map (kept for lazy migration of old scenes). */
inline constexpr const char *kLegacyMapKey = "map";

/** Current map: scene UUID -> profile (name keys are still accepted). */
inline constexpr const char *kMapKey = "map_v2";

/** True for a canonical 36-character UUID. */
bool looks_like_uuid(const std::string &value);

/** Reads one map from the OBS profile config. Invalid JSON yields an empty map. */
std::map<std::string, int> read_map(config_t *cfg, const char *key);

/** Writes and saves one map in the OBS profile config. */
bool write_map(config_t *cfg, const char *key, const std::map<std::string, int> &map);

} // namespace topscreen::scene_profiles::detail
