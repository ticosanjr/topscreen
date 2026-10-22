#pragma once

#include <atomic>

#include <obs-module.h>

#include "ts-hotkey-actions.hpp"

namespace topscreen {

struct FilterControl;

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
	/** Activates a profile (0..9) and queues saving into the current profile. */
	virtual void select_profile(int index) = 0;
	virtual void save_profile() = 0;
	virtual void toggle_highlight() = 0;
	virtual void toggle_spotlight() = 0;
	virtual void toggle_magnifier() = 0;

	/**
	 * The FilterControl implemented by the same object, when available.
	 * The registry uses it to make hotkeys follow the dock's selected target.
	 */
	virtual FilterControl *filter_control() { return nullptr; }
};

struct HotkeyBinding {
	int action = 0;
};

void ts_hotkey_callback(void *data, obs_hotkey_id id, obs_hotkey_t *key, bool pressed);

/**
 * Process-wide hotkeys.
 *
 * OBS marks every filter source as private, so obs_hotkey_register_source()
 * always fails for filters (OBS_INVALID_HOTKEY_ID on every platform). The 21
 * actions are therefore registered ONCE as frontend hotkeys and dispatched to
 * the filter selected in the dock (or the first active one). Refcounted across
 * filter instances so creating/destroying filters never leaks or duplicates entries.
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
