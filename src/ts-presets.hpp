#pragma once

#include <string>
#include <vector>

#include "core/config.hpp"
#include "core/preset.hpp"

namespace topscreen {

/**
 * Loads, applies and stores looks/presets.
 *
 * Bundled looks live in data/presets and are read-only. User looks are single
 * JSON files in the plugin config directory (`looks/`), so they can be copied
 * between machines and are included in the preset list/hotkey cycle.
 */
class PresetStore {
public:
	/** Overrides the writable looks directory (tests and portable setups). */
	void set_user_dir(std::string dir) { user_dir_ = std::move(dir); }

	/** Loads bundled and user looks. Returns how many were loaded. */
	int load();

	int count() const { return static_cast<int>(entries_.size()); }
	int current() const { return current_; }
	const std::string &name(int index) const;
	bool is_user(int index) const;

	/** Applies look `index` onto the config. Returns false if invalid. */
	bool apply(int index, core::Config *cfg);

	/** Advances/goes back cyclically. Returns the new index (-1 if empty). */
	int next();
	int prev();

	/** Forgets the last applied look (a profile switch changes the look). */
	void clear_current() { current_ = -1; }

	/** Resolved writable directory (empty until load/save resolves it). */
	const std::string &user_dir() const { return user_dir_; }

	/** Writes the current config as one user look. Returns its index or -1. */
	int save_user(const std::string &name, const core::Config &cfg, std::string *error);

	/** Deletes one user look. Bundled looks are rejected. */
	bool delete_user(int index, std::string *error);

private:
	struct Entry {
		core::Preset preset;
		std::string path;
		bool user = false;
	};

	std::string resolve_user_dir();
	std::vector<Entry> entries_;
	std::string user_dir_;
	int current_ = -1;
};

} // namespace topscreen
