#include "ts-scene-profiles.hpp"
#include "ts-scene-profiles-internal.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>

#include <map>
#include <set>

#include "core/json.hpp"
#include "core/keys.hpp"
#include "core/profiles.hpp"

namespace topscreen::scene_profiles {

namespace {

std::string source_uuid(const obs_source_t *scene)
{
	if (!scene)
		return {};
	const char *uuid = obs_source_get_uuid(scene);
	return uuid ? uuid : "";
}

std::string source_name(const obs_source_t *scene)
{
	if (!scene)
		return {};
	const char *name = obs_source_get_name(scene);
	return name ? name : "";
}

std::map<std::string, std::string> current_scene_names()
{
	std::map<std::string, std::string> out;
	obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	for (size_t i = 0; i < scenes.sources.num; ++i) {
		obs_source_t *scene = scenes.sources.array[i];
		const std::string uuid = source_uuid(scene);
		const std::string name = source_name(scene);
		if (!uuid.empty() && !name.empty())
			out[uuid] = name;
	}
	obs_frontend_source_list_free(&scenes);
	return out;
}

} // namespace

bool enabled()
{
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return true;
	const bool has = config_has_user_value(cfg, kSection, "enabled") ||
			 config_has_default_value(cfg, kSection, "enabled");
	return has ? config_get_bool(cfg, kSection, "enabled") : true;
}

void set_enabled(bool on)
{
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return;
	config_set_bool(cfg, kSection, "enabled", on);
	config_save_safe(cfg, "tmp", nullptr);
	blog(LOG_INFO, "[TopScreen] scene profile switching: %s", on ? "on" : "off");
}

int get(const std::string &scene)
{
	if (scene.empty())
		return -1;
	config_t *cfg = obs_frontend_get_profile_config();
	const auto map = detail::read_map(cfg, detail::kMapKey);
	const auto it = map.find(scene);
	if (it != map.end())
		return it->second;
	const auto legacy = detail::read_map(cfg, detail::kLegacyMapKey);
	const auto old = legacy.find(scene);
	return old == legacy.end() ? -1 : old->second;
}

int get(const struct obs_source *scene)
{
	if (!scene)
		return -1;
	const std::string uuid = source_uuid(scene);
	const std::string name = source_name(scene);
	if (!uuid.empty()) {
		config_t *cfg = obs_frontend_get_profile_config();
		auto map = detail::read_map(cfg, detail::kMapKey);
		const auto it = map.find(uuid);
		if (it != map.end())
			return it->second;

		/* Lazy migration: an assignment created before UUID storage still
		 * lives under the scene name. Move it to the stable UUID now, so a
		 * later scene rename no longer loses the mapping. */
		auto legacy = detail::read_map(cfg, detail::kLegacyMapKey);
		const auto old = legacy.find(name);
		if (old != legacy.end()) {
			const int profile = old->second;
			map[uuid] = profile;
			detail::write_map(cfg, detail::kMapKey, map);
			legacy.erase(old);
			detail::write_map(cfg, detail::kLegacyMapKey, legacy);
			return profile;
		}
	}
	return get(name);
}

bool assign(const std::string &scene, int profile)
{
	if (scene.empty() || profile < 0 || profile >= core::keys::kProfileCount)
		return false;
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return false;
	auto map = detail::read_map(cfg, detail::kMapKey);
	map[scene] = profile;
	detail::write_map(cfg, detail::kMapKey, map);
	blog(LOG_INFO, "[TopScreen] scene profile: %s -> %s", scene.c_str(),
	     core::profile_label(profile));
	return true;
}

bool assign(struct obs_source *scene, int profile)
{
	if (!scene)
		return false;
	const std::string uuid = source_uuid(scene);
	const std::string name = source_name(scene);
	if (uuid.empty())
		return assign(name, profile);

	if (profile < 0 || profile >= core::keys::kProfileCount)
		return false;
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return false;

	auto map = detail::read_map(cfg, detail::kMapKey);
	map[uuid] = profile;
	detail::write_map(cfg, detail::kMapKey, map);

	/* Remove the legacy name entry so the same scene does not appear twice. */
	auto legacy = detail::read_map(cfg, detail::kLegacyMapKey);
	if (!name.empty() && legacy.erase(name) > 0)
		detail::write_map(cfg, detail::kLegacyMapKey, legacy);

	blog(LOG_INFO, "[TopScreen] scene profile: %s -> %s", name.c_str(),
	     core::profile_label(profile));
	return true;
}

bool clear(const std::string &scene)
{
	if (scene.empty())
		return false;
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return false;

	bool changed = false;
	for (const char *key : {detail::kMapKey, detail::kLegacyMapKey}) {
		auto map = detail::read_map(cfg, key);
		if (map.erase(scene) > 0) {
			detail::write_map(cfg, key, map);
			changed = true;
		}
	}
	if (changed)
		blog(LOG_INFO, "[TopScreen] scene profile cleared: %s", scene.c_str());
	return changed;
}

bool clear(struct obs_source *scene)
{
	if (!scene)
		return false;
	const std::string uuid = source_uuid(scene);
	const std::string name = source_name(scene);
	bool changed = false;
	if (!uuid.empty())
		changed = clear(uuid) || changed;
	if (!name.empty() && name != uuid)
		changed = clear(name) || changed;
	return changed;
}

std::vector<std::pair<std::string, int>> list()
{
	config_t *cfg = obs_frontend_get_profile_config();
	std::map<std::string, int> display;

	for (const auto &entry : detail::read_map(cfg, detail::kLegacyMapKey))
		display[entry.first] = entry.second;

	const auto names = current_scene_names();
	for (const auto &entry : detail::read_map(cfg, detail::kMapKey)) {
		const auto it = names.find(entry.first);
		display[it == names.end() ? entry.first : it->second] = entry.second;
	}

	std::vector<std::pair<std::string, int>> out;
	out.reserve(display.size());
	for (const auto &entry : display)
		out.emplace_back(entry.first, entry.second);
	return out;
}

bool clear_all()
{
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return false;
	config_remove_value(cfg, kSection, detail::kMapKey);
	config_remove_value(cfg, kSection, detail::kLegacyMapKey);
	config_save_safe(cfg, "tmp", nullptr);
	blog(LOG_INFO, "[TopScreen] all scene profile assignments cleared");
	return true;
}

void prune(const std::vector<std::string> &scene_names)
{
	/* An empty list can be a transient state while OBS changes collections;
	 * never interpret it as "every scene was deleted". */
	if (scene_names.empty())
		return;
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return;

	std::set<std::string> valid(scene_names.begin(), scene_names.end());
	bool changed = false;

	auto legacy = detail::read_map(cfg, detail::kLegacyMapKey);
	for (auto it = legacy.begin(); it != legacy.end();) {
		if (valid.find(it->first) == valid.end()) {
			it = legacy.erase(it);
			changed = true;
		} else {
			++it;
		}
	}

	/* UUID-keyed entries are pruned by prune_uuids(); do not remove them just
	 * because their key is not a scene name. */
	auto map = detail::read_map(cfg, detail::kMapKey);
	for (auto it = map.begin(); it != map.end();) {
		if (!detail::looks_like_uuid(it->first) && valid.find(it->first) == valid.end()) {
			it = map.erase(it);
			changed = true;
		} else {
			++it;
		}
	}

	if (changed) {
		detail::write_map(cfg, detail::kLegacyMapKey, legacy);
		detail::write_map(cfg, detail::kMapKey, map);
		blog(LOG_INFO, "[TopScreen] removed scene profile assignments for deleted scenes");
	}
}

std::string export_json()
{
	config_t *cfg = obs_frontend_get_profile_config();
	std::map<std::string, int> merged = detail::read_map(cfg, detail::kLegacyMapKey);
	for (const auto &entry : detail::read_map(cfg, detail::kMapKey))
		merged[entry.first] = entry.second;

	/* A backup must survive a different machine/scene collection: write scene
	 * names instead of local UUIDs whenever the scene is in this collection. */
	const auto names = current_scene_names();
	obs_data_t *data = obs_data_create();
	for (const auto &entry : merged) {
		std::string key = entry.first;
		if (detail::looks_like_uuid(key)) {
			const auto it = names.find(key);
			if (it != names.end() && !it->second.empty())
				key = it->second;
		}
		obs_data_set_int(data, key.c_str(), entry.second);
	}
	const char *json = obs_data_get_json(data);
	std::string out = json ? json : "{}";
	obs_data_release(data);
	return out;
}

bool import_json(const std::string &json_text)
{
	core::json::Value root;
	std::string error;
	if (!core::json::parse(json_text, &root, &error) || !root.is_object())
		return false;

	std::map<std::string, int> map;
	for (const auto &entry : *root.object) {
		if (!entry.second.is_number())
			continue;
		const long long profile = static_cast<long long>(entry.second.as_number());
		if (profile >= 0 && profile < core::keys::kProfileCount)
			map[entry.first] = static_cast<int>(profile);
	}

	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return false;
	detail::write_map(cfg, detail::kMapKey, map);
	config_remove_value(cfg, kSection, detail::kLegacyMapKey);
	config_save_safe(cfg, "tmp", nullptr);
	blog(LOG_INFO, "[TopScreen] scene profile assignments imported (%zu)",
	     map.size());
	return true;
}

bool import_from_bundle(const std::string &bundle_json)
{
	obs_data_t *bundle = obs_data_create_from_json(bundle_json.c_str());
	if (!bundle)
		return false;
	const char *scenes = obs_data_get_string(bundle, "scene_profiles");
	const bool ok = !scenes || !*scenes || import_json(scenes);
	obs_data_release(bundle);
	return ok;
}

void prune_uuids(const std::vector<std::string> &scene_uuids)
{
	/* OBS versions that do not expose scene UUIDs would otherwise erase every
	 * UUID-keyed assignment on a collection change. */
	if (scene_uuids.empty())
		return;
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return;

	std::set<std::string> valid(scene_uuids.begin(), scene_uuids.end());
	auto map = detail::read_map(cfg, detail::kMapKey);
	bool changed = false;
	for (auto it = map.begin(); it != map.end();) {
		if (detail::looks_like_uuid(it->first) && valid.find(it->first) == valid.end()) {
			it = map.erase(it);
			changed = true;
		} else {
			++it;
		}
	}
	if (changed) {
		detail::write_map(cfg, detail::kMapKey, map);
		blog(LOG_INFO, "[TopScreen] removed scene profile assignments for deleted scenes");
	}
}

} // namespace topscreen::scene_profiles
