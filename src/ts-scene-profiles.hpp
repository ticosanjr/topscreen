#pragma once

#include <string>
#include <utility>
#include <vector>

namespace topscreen::scene_profiles {

/** OBS profile-config section used for the scene -> TopScreen profile map. */
inline constexpr const char *kSection = "TopScreen.Scenes";

/** Returns the assigned profile (0..9), or -1 when the scene has none. */
int get(const std::string &scene);

/** Assigns a profile to a scene and persists the OBS profile config. */
bool assign(const std::string &scene, int profile);

/** Removes the assignment for one scene. */
bool clear(const std::string &scene);

/** All scene -> profile assignments, sorted by scene name. */
std::vector<std::pair<std::string, int>> list();

/** Removes every scene assignment. */
bool clear_all();

/** Drops assignments whose scene no longer exists in the collection. */
void prune(const std::vector<std::string> &scene_names);

} // namespace topscreen::scene_profiles
