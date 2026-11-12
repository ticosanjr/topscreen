#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "hotkeys/ts-hotkeys.hpp"

namespace topscreen {

/**
 * Filter control interface, exposed to the dock (UI thread).
 *
 * All implementations must be safe to call from any thread:
 * the mutations are queued and applied in video_tick (video thread).
 */
struct FilterControl {
	virtual ~FilterControl() = default;

	/** Display name of the OBS source that owns this filter (may be empty). */
	virtual std::string source_name() const = 0;

	virtual bool effects_enabled() const = 0;
	virtual void set_effects_enabled(bool on) = 0;

	virtual int preset_count() const = 0;
	virtual std::string preset_name(int index) const = 0;
	virtual int current_preset() const = 0;
	virtual void apply_preset(int index) = 0;

	virtual int input_tier() const = 0;
	/** Origin of the captured monitor in global coordinates (0,0 when unknown). */
	virtual void monitor_origin(double *x, double *y) const = 0;

	/* Profiles (10 fixed slots). select_profile/save_profile also exist in
	 * HotkeyContext (same signature); the instance implements them once. */
	virtual int profile_count() const = 0;
	virtual int current_profile() const = 0;
	virtual void select_profile(int index) = 0;
	virtual void save_profile() = 0;
	virtual void toggle_highlight() = 0;
	/** Ends a pinned highlight/zoom on scene changes/build exit. */
	virtual void cancel_highlight() = 0;
	/** Test/diagnostic: the independent hotkey highlight is currently active. */
	virtual bool hotkey_highlight_active() const = 0;
	/** Test/diagnostic: resolved module path of the loaded click sound. */
	virtual std::string loaded_sound_path(bool right) const = 0;

	/* Profiles backup: the filter does the file I/O on the video thread and
	 * publishes the last result (0 = none, 1 = export ok, 2 = export failed,
	 * 3 = import ok, 4 = import failed). */
	/** `scene_profiles_json` is computed on the UI thread and only written. */
	virtual void export_profiles(const std::string &path,
				     const std::string &scene_profiles_json) = 0;
	virtual void import_profiles(const std::string &json_text) = 0;
	virtual int profile_io_result() const = 0;
};

/** One active filter, as shown in the dock's target selector. */
struct FilterEntry {
	FilterControl *control = nullptr;
	std::string name;
};

/** Registers/unregisters an active filter. */
void registry_add(FilterControl *control);
void registry_remove(FilterControl *control);

/** Snapshot of the active filters in registration order (dock selector). */
std::vector<FilterEntry> registry_list();

/** Changes whenever the active filter list changes (add/remove). The dock
 * uses it to avoid rebuilding the target selector every refresh tick. */
std::uint64_t registry_revision();

/**
 * Selects the dock/hotkey target. Returns false when the control is no longer
 * registered (e.g. its source was deleted between two dock refreshes).
 */
bool registry_select(FilterControl *control);

/** Selected active filter; falls back to the first one when none is selected. */
FilterControl *registry_first();

/**
 * Hotkey targets. OBS marks every filter source as private, so
 * obs_hotkey_register_source() always fails for filters (returns
 * OBS_INVALID_HOTKEY_ID on every platform). Hotkeys are therefore registered
 * once, process-wide, as *frontend* hotkeys and dispatched to the filter
 * selected in the dock (or the first one when nothing is selected).
 */
void hotkey_target_add(HotkeyContext *target);
void hotkey_target_remove(HotkeyContext *target);
HotkeyContext *hotkey_target_first();

} // namespace topscreen
