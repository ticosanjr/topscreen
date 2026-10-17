#pragma once

#include <string>

namespace topscreen {

/**
 * Filter control interface, exposed to the dock (UI thread).
 *
 * All implementations must be safe to call from any thread:
 * the mutations are queued and applied in video_tick (video thread).
 */
struct FilterControl {
	virtual ~FilterControl() = default;

	virtual bool effects_enabled() const = 0;
	virtual void set_effects_enabled(bool on) = 0;

	virtual int preset_count() const = 0;
	virtual std::string preset_name(int index) const = 0;
	virtual int current_preset() const = 0;
	virtual void apply_preset(int index) = 0;

	virtual int input_tier() const = 0;

	/* Profiles (4 fixed slots). select_profile/save_profile also exist in
	 * HotkeyContext (same signature); the instance implements them once. */
	virtual int profile_count() const = 0;
	virtual int current_profile() const = 0;
	virtual void select_profile(int index) = 0;
	virtual void save_profile() = 0;
};

/** Registers/unregisters an active filter. */
void registry_add(FilterControl *control);
void registry_remove(FilterControl *control);

/** First active filter (or nullptr). Used by the dock. */
FilterControl *registry_first();

} // namespace topscreen
