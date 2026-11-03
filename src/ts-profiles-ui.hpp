#pragma once

namespace topscreen {

/** Settings keys for the Properties fallback (no Qt dock available). */
inline constexpr const char *kProfileExportPath = "profile_export_path";
inline constexpr const char *kProfileImportPath = "profile_import_path";

/**
 * Properties buttons: read the path selected in the source settings and queue
 * the operation on the filter. `filter` is the opaque filter context passed as
 * the button's private data. Return false when the path is empty/unreadable.
 */
bool profiles_export_from_properties(void *filter);
bool profiles_import_from_properties(void *filter);

/** Queues a one-time restore of profiles A..J to the built-in seeds. */
bool profiles_reset_from_properties(void *filter);

} // namespace topscreen
