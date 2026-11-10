#pragma once

#include <cstring>

/**
 * Hotkey actions and their process-wide OBS IDs.
 *
 * This header is intentionally free of libobs includes so the naming policy
 * (every ID starts with "TopScreen_") is unit-tested by the core test binary.
 *
 * The indexes are stable: append new actions at the end and never reorder the
 * existing ones, otherwise saved hotkey assignments stop matching.
 */
namespace topscreen {

enum HotkeyAction {
	HK_ToggleEffects = 0,
	HK_ToggleBackground,
	HK_ZoomIn,
	HK_ZoomOut,
	HK_ResetZoom,
	HK_NextPreset,
	HK_PrevPreset,
	HK_ProfileA,
	HK_ProfileB,
	HK_ProfileC,
	HK_ProfileD,
	HK_ProfileE,
	HK_ProfileF,
	HK_ProfileG,
	HK_ProfileH,
	HK_ProfileI,
	HK_ProfileJ,
	HK_ProfileSave,
	HK_ToggleHighlight,
	HK_ToggleSpotlight,
	HK_ToggleMagnifier,
	HK_ToggleCursor,
	HK_ProfileNext,
	HK_ProfilePrev,
	HK_ToggleClickSound,
	HK_Count,
};

/** Mandatory prefix for every hotkey ID (easy to find in OBS). */
inline constexpr const char *kHotkeyPrefix = "TopScreen_";

/** IDs as shown/registered in Settings -> Hotkeys. Index = HotkeyAction. */
inline constexpr const char *kHotkeyNames[HK_Count] = {
	"TopScreen_ToggleEffects",  "TopScreen_ToggleBackground", "TopScreen_ZoomIn",
	"TopScreen_ZoomOut",        "TopScreen_ResetZoom",        "TopScreen_NextPreset",
	"TopScreen_PrevPreset",     "TopScreen_ProfileA",         "TopScreen_ProfileB",
	"TopScreen_ProfileC",       "TopScreen_ProfileD",         "TopScreen_ProfileE",
	"TopScreen_ProfileF",       "TopScreen_ProfileG",         "TopScreen_ProfileH",
	"TopScreen_ProfileI",       "TopScreen_ProfileJ",         "TopScreen_ProfileSave",
	"TopScreen_ToggleHighlight",
	"TopScreen_ToggleSpotlight",
	"TopScreen_ToggleMagnifier",
	"TopScreen_ToggleCursor",
	"TopScreen_ProfileNext",
	"TopScreen_ProfilePrev",
	"TopScreen_ToggleClickSound",
};

/** Locale keys, index = HotkeyAction. */
inline constexpr const char *kHotkeyLabelKeys[HK_Count] = {
	"Hotkey.ToggleEffects",
	"Hotkey.ToggleBackground",
	"Hotkey.ZoomIn",
	"Hotkey.ZoomOut",
	"Hotkey.ResetZoom",
	"Hotkey.NextPreset",
	"Hotkey.PrevPreset",
	"Hotkey.ProfileA",
	"Hotkey.ProfileB",
	"Hotkey.ProfileC",
	"Hotkey.ProfileD",
	"Hotkey.ProfileE",
	"Hotkey.ProfileF",
	"Hotkey.ProfileG",
	"Hotkey.ProfileH",
	"Hotkey.ProfileI",
	"Hotkey.ProfileJ",
	"Hotkey.ProfileSave",
	"Hotkey.ToggleHighlight",
	"Hotkey.ToggleSpotlight",
	"Hotkey.ToggleMagnifier",
	"Hotkey.ToggleCursor",
	"Hotkey.ProfileNext",
	"Hotkey.ProfilePrev",
	"Hotkey.ToggleClickSound",
};

inline constexpr int kHotkeyCount = HK_Count;

/**
 * Returns the localized label, or the literal ID when OBS could not load the
 * locale (obs_module_text then returns the key itself). Without this fallback
 * Settings -> Hotkeys shows "Hotkey.ProfileA" instead of "TopScreen_ProfileA".
 */
inline const char *hotkey_label_or_id(const char *translated, const char *id)
{
	if (!translated || translated[0] == '\0' || std::strncmp(translated, "Hotkey.", 7) == 0)
		return id;
	return translated;
}

} // namespace topscreen
