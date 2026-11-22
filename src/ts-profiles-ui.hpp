#pragma once

namespace topscreen {

/** Settings keys for the Properties fallback (no Qt dock available). */
inline constexpr const char *kProfileExportPath = "profile_export_path";
inline constexpr const char *kProfileImportPath = "profile_import_path";
inline constexpr const char *kProfileNameInput = "profile_name_input";
inline constexpr const char *kProfileDuplicateTarget = "profile_duplicate_target";
inline constexpr const char *kProfileConfirmOverwrite = "profile_confirm_overwrite";
inline constexpr const char *kSceneSwitchEnabled = "scene_switch_enabled";

/**
 * Properties buttons: read the path selected in the source settings and queue
 * the operation on the filter. `filter` is the opaque filter context passed as
 * the button's private data. Return false when the path is empty/unreadable.
 */
bool profiles_export_from_properties(void *filter);
bool profiles_import_from_properties(void *filter);

/** Queues a one-time restore of profiles A..J to the built-in seeds. */
bool profiles_reset_from_properties(void *filter);

/** Renames the active profile with the text in `profile_name_input`. */
bool profiles_rename_from_properties(void *filter);

/** Copies the active profile into the slot in `profile_duplicate_target`. */
bool profiles_duplicate_from_properties(void *filter);

/** Restores the active profile to the state it had when it was activated. */
bool profiles_revert_from_properties(void *filter);

/** True when the user ticked the "overwrites A-J" confirmation. */
bool profiles_overwrite_confirmed(void *filter);

} // namespace topscreen
