#include "ts-profile-bundle.hpp"

#include "core/keys.hpp"
#include "ts-profile-store.hpp"
#include "ts-version.hpp"

namespace topscreen::profile_bundle {

namespace {

void set_error(std::string *error, const char *message)
{
	if (error)
		*error = message ? message : "";
}

} // namespace

bool export_to_file(obs_data_t *source_settings, int current, const std::string &path,
		    const std::string &scene_profiles_json, std::string *error)
{
	if (!source_settings || path.empty()) {
		set_error(error, "empty path");
		return false;
	}

	obs_data_t *bundle = obs_data_create();
	obs_data_set_int(bundle, "topscreen_profiles", 1);
	obs_data_set_string(bundle, "plugin", "TopScreen");
	obs_data_set_string(bundle, "version", kVersion);
	obs_data_set_int(bundle, "current", current);
	/* Kept as a string so older builds ignore it and the map can be replaced
	 * without touching the slot schema. */
	if (!scene_profiles_json.empty())
		obs_data_set_string(bundle, "scene_profiles", scene_profiles_json.c_str());
	for (int i = 0; i < core::keys::kProfileCount; ++i) {
		/* Normalize every slot: older sources can miss fields added in a
		 * newer version, and the exported bundle must always be complete. */
		profile_store::store_slot(bundle, i,
					  profile_store::load_slot(source_settings, i));
		profile_store::store_name(bundle, i,
					  profile_store::load_name(source_settings, i));
	}

	const bool ok = obs_data_save_json_pretty_safe(bundle, path.c_str(), "tmp", "bak");
	obs_data_release(bundle);
	if (!ok)
		set_error(error, "could not write the profiles file");
	return ok;
}

bool import_from_text(const std::string &json_text, obs_data_t *target_settings,
		      core::ProfileBundleInfo *info, std::string *scene_profiles_json,
		      std::string *error)
{
	if (!target_settings) {
		set_error(error, "missing target settings");
		return false;
	}

	core::ProfileBundleInfo parsed;
	if (!core::profile_bundle_validate(json_text, &parsed, error))
		return false;

	obs_data_t *bundle = obs_data_create_from_json(json_text.c_str());
	if (!bundle) {
		set_error(error, "could not parse the JSON");
		return false;
	}

	for (int i = 0; i < core::keys::kProfileCount; ++i) {
		/* Missing slots keep the seed; missing fields keep the seed's value
		 * instead of becoming 0/false. This keeps old/new bundles valid. */
		profile_store::store_slot(target_settings, i,
					  profile_store::load_slot(bundle, i));
		profile_store::store_name(target_settings, i,
					  profile_store::load_name(bundle, i));
	}
	obs_data_set_int(target_settings, core::keys::kProfileCurrent, parsed.current);

	if (info)
		*info = parsed;
	if (scene_profiles_json) {
		const char *scenes = obs_data_get_string(bundle, "scene_profiles");
		*scene_profiles_json = scenes ? scenes : "";
	}
	obs_data_release(bundle);
	return true;
}

} // namespace topscreen::profile_bundle
