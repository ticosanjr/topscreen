#pragma once

#include "config.hpp"
#include "json.hpp"

#include <string>

namespace topscreen::core {

/**
 * Preset: JSON envelope with metadata and a `settings` block containing the
 * configuration keys (the same keys as obs_data - see core/keys.hpp).
 */
struct Preset {
	std::string name;
	std::string description;

	/** Applies the preset values onto the config (missing values are kept). */
	bool apply(Config *cfg, std::string *error) const;

	/** Parses the JSON text of a preset. */
	static bool from_json(const std::string &text, Preset *out, std::string *error);

	/** `settings` object of the preset (for inspection/tests). */
	const json::Value &settings() const { return settings_; }

private:
	json::Value settings_;
};

} // namespace topscreen::core
