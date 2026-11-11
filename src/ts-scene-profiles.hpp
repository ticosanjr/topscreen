#pragma once

#include <string>
#include <utility>
#include <vector>

struct obs_source;

namespace topscreen::scene_profiles {

/** OBS profile-config section used for the scene -> TopScreen profile map. */
inline constexpr const char *kSection = "TopScreen.Scenes";

/** Returns the assigned profile (0..9), or -1 when the scene has none. */
int get(const std::string &scene);

/** UUID-aware lookup: prefers the scene UUID and falls back to its name. */
int get(const struct obs_source *scene);

/** Assigns a profile to a scene and persists the OBS profile config. */
bool assign(const std::string &scene, int profile);

/** UUID-aware assignment: stores the stable scene UUID when available. */
bool assign(struct obs_source *scene, int profile);

/** Removes the assignment for one scene. */
bool clear(const std::string &scene);

/** UUID-aware removal: clears both the UUID and the legacy name entry. */
bool clear(struct obs_source *scene);

/** All scene -> profile assignments, sorted by scene name. */
std::vector<std::pair<std::string, int>> list();

/** Removes every scene assignment. */
bool clear_all();

/** Drops assignments whose scene no longer exists in the collection. */
void prune(const std::vector<std::string> &scene_names);

/** Drops UUID-keyed assignments whose scene no longer exists. */
void prune_uuids(const std::vector<std::string> &scene_uuids);

/** Serializes the raw scene -> profile map (used by the profiles backup). */
std::string export_json();

/** Replaces the scene -> profile map from a backup. Invalid input is ignored. */
bool import_json(const std::string &json_text);

/** Restores the optional scene map from a profiles bundle (UI thread). */
bool import_from_bundle(const std::string &bundle_json);

} // namespace topscreen::scene_profiles
