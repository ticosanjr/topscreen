#pragma once

#include <string>
#include <vector>

#include "core/config.hpp"
#include "core/preset.hpp"

namespace topscreen {

/**
 * Loads and applies built-in presets (.json files in data/presets).
 *
 * The files are resolved via obs_module_file(), so they work both
 * installed and in portable mode.
 */
class PresetStore {
public:
	/** Loads the known presets. Returns how many were loaded. */
	int load();

	int count() const { return static_cast<int>(presets_.size()); }
	int current() const { return current_; }
	const std::string &name(int index) const;

	/** Applies preset `index` onto the config. Returns false if invalid. */
	bool apply(int index, core::Config *cfg);

	/** Advances/goes back cyclically. Returns the new index (-1 if empty). */
	int next();
	int prev();

	/** Forgets the last applied preset (a profile switch changes the look). */
	void clear_current() { current_ = -1; }

private:
	std::vector<core::Preset> presets_;
	int current_ = -1;
};

} // namespace topscreen
