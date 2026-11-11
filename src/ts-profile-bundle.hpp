#pragma once

#include <obs-module.h>

#include <string>

#include "core/profiles_io.hpp"

namespace topscreen::profile_bundle {

/**
 * Writes a complete profiles backup (metadata + all ten normalized slots).
 *
 * `source_settings` is the OBS source settings object; every slot is completed
 * from that slot's seed before being written, so a bundle exported by an older
 * source is still valid for a newer build.
 *
 * Returns false and fills `error` when the path is empty or the file cannot be
 * written.
 */
bool export_to_file(obs_data_t *source_settings, int current, const std::string &path,
		    const std::string &scene_profiles_json, std::string *error);

/**
 * Validates `json_text` and copies its slots into `target_settings`.
 *
 * Missing slots keep their seed; missing fields keep the seed value instead of
 * becoming 0/false, so old and new bundles stay compatible. On success the
 * current profile is written to the target settings and `info` describes the
 * imported bundle.
 *
 * Returns false and fills `error` when the text is not a valid bundle.
 */
bool import_from_text(const std::string &json_text, obs_data_t *target_settings,
		      core::ProfileBundleInfo *info, std::string *scene_profiles_json,
		      std::string *error);

} // namespace topscreen::profile_bundle
