#include "profiles_io.hpp"

#include <cmath>

#include "json.hpp"
#include "keys.hpp"

namespace topscreen::core {

namespace {

bool fail(std::string *error, const std::string &message)
{
	if (error)
		*error = message;
	return false;
}

} // namespace

bool profile_bundle_validate(const std::string &json_text, ProfileBundleInfo *out,
			     std::string *error)
{
	if (!json_text.empty() && json_text.size() > kMaxProfileBundleBytes)
		return fail(error, "profiles file is too large");

	json::Value root;
	std::string parse_error;
	if (!json::parse(json_text, &root, &parse_error))
		return fail(error, "invalid JSON: " + parse_error);

	if (!root.is_object())
		return fail(error, "not a TopScreen profiles file (root is not an object)");

	const json::Value *marker = root.find("topscreen_profiles");
	if (!marker || !marker->is_number() || marker->as_number() != 1.0)
		return fail(error, "not a TopScreen profiles file (missing format marker 1)");

	const json::Value *plugin = root.find("plugin");
	if (!plugin || plugin->as_string() != "TopScreen")
		return fail(error, "not a TopScreen profiles file (plugin marker)");

	double cur = 0.0;
	if (const json::Value *current = root.find("current")) {
		if (!current->is_number())
			return fail(error, "current profile index is not a number");
		cur = current->as_number();
		if (std::trunc(cur) != cur || cur < 0.0 ||
		    cur >= static_cast<double>(keys::kProfileCount))
			return fail(error, "current profile index is out of range");
	}

	int slots = 0;
	for (int i = 0; i < keys::kProfileCount; ++i) {
		const json::Value *slot = root.find(keys::kProfileSlots[i]);
		if (!slot)
			continue;
		if (!slot->is_object())
			return fail(error, std::string("profile slot is not an object: ") +
						   keys::kProfileSlots[i]);
		++slots;
	}
	if (slots == 0)
		return fail(error, "profiles file has no profile slots");

	if (out) {
		out->current = static_cast<int>(cur);
		out->slot_count = slots;
	}
	return true;
}

} // namespace topscreen::core
