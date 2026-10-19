#pragma once

#include <atomic>

#include <obs-module.h>

namespace topscreen {

/** Hotkey actions (stable index - do not reorder without migrating configs). */
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
	HK_Count,
};

/** Interface implemented by the filter to react to hotkeys. */
struct HotkeyContext {
	virtual ~HotkeyContext() = default;
	virtual void toggle_effects() = 0;
	virtual void toggle_background() = 0;
	virtual void zoom_in() = 0;
	virtual void zoom_out() = 0;
	virtual void reset_zoom() = 0;
	virtual void next_preset() = 0;
	virtual void prev_preset() = 0;
	/** Activates a profile (0..3) and queues saving into the current profile. */
	virtual void select_profile(int index) = 0;
	virtual void save_profile() = 0;
};

struct HotkeyBinding {
	int action = 0;
};

void ts_hotkey_callback(void *data, obs_hotkey_id id, obs_hotkey_t *key, bool pressed);

/**
 * Process-wide hotkeys.
 *
 * OBS marks every filter source as private, so obs_hotkey_register_source()
 * always fails for filters (OBS_INVALID_HOTKEY_ID on every platform). The 18
 * actions are therefore registered ONCE as frontend hotkeys and dispatched to
 * the first active filter (same rule as the dock). Refcounted across filter
 * instances so creating/destroying filters never leaks or duplicates entries.
 */
class Hotkeys {
public:
	void register_all(HotkeyContext *ctx);
	void unregister_all(HotkeyContext *ctx);

private:
	static std::atomic<int> refs_;
	static HotkeyBinding bindings_[HK_Count];
	static obs_hotkey_id ids_[HK_Count];
};

} // namespace topscreen
