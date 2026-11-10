#pragma once

#include <string>

namespace topscreen::scene_profiles {

/** OBS profile-config section used for the scene -> TopScreen profile map. */
inline constexpr const char *kSection = "TopScreen.Scenes";

/** Returns the assigned profile (0..9), or -1 when the scene has none. */
int get(const std::string &scene);

/** Assigns a profile to a scene and persists the OBS profile config. */
bool assign(const std::string &scene, int profile);

/** Removes the assignment for one scene. */
bool clear(const std::string &scene);

} // namespace topscreen::scene_profiles
