#include "ts-scene-profiles.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>

#include <cctype>
#include <map>
#include <set>

#include "core/json.hpp"
#include "core/keys.hpp"
#include "core/profiles.hpp"

namespace topscreen::scene_profiles {

namespace {

constexpr const char *kLegacyMapKey = "map";
constexpr const char *kMapKey = "map_v2";

bool looks_like_uuid(const std::string &value)
{
	if (value.size() != 36)
		return false;
	for (size_t i = 0; i < value.size(); ++i) {
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			if (value[i] != '-')
				return false;
		} else if (!std::isxdigit(static_cast<unsigned char>(value[i]))) {
			return false;
		}
	}
	return true;
}

std::map<std::string, int> read_map(config_t *cfg, const char *key)
{
	std::map<std::string, int> out;
	if (!cfg)
		return out;
	const char *json = config_get_string(cfg, kSection, key);
	if (!json || !*json)
		return out;

	core::json::Value root;
	std::string error;
	if (!core::json::parse(json, &root, &error) || !root.is_object())
		return out;
	for (const auto &entry : *root.object) {
		if (!entry.second.is_number())
			continue;
		const long long profile = static_cast<long long>(entry.second.as_number());
		if (profile >= 0 && profile < core::keys::kProfileCount)
			out[entry.first] = static_cast<int>(profile);
	}
	return out;
}

bool write_map(config_t *cfg, const char *key, const std::map<std::string, int> &map)
{
	if (!cfg)
		return false;
	obs_data_t *data = obs_data_create();
	for (const auto &entry : map)
		obs_data_set_int(data, entry.first.c_str(), entry.second);
	const char *json = obs_data_get_json(data);
	config_set_string(cfg, kSection, key, json ? json : "{}");
	obs_data_release(data);
	config_save_safe(cfg, "tmp", nullptr);
	return true;
}

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

int get(const std::string &scene)
{
	if (scene.empty())
		return -1;
	config_t *cfg = obs_frontend_get_profile_config();
	const auto map = read_map(cfg, kMapKey);
	const auto it = map.find(scene);
	if (it != map.end())
		return it->second;
	const auto legacy = read_map(cfg, kLegacyMapKey);
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
		auto map = read_map(cfg, kMapKey);
		const auto it = map.find(uuid);
		if (it != map.end())
			return it->second;

		/* Lazy migration: an assignment created before UUID storage still
		 * lives under the scene name. Move it to the stable UUID now, so a
		 * later scene rename no longer loses the mapping. */
		auto legacy = read_map(cfg, kLegacyMapKey);
		const auto old = legacy.find(name);
		if (old != legacy.end()) {
			const int profile = old->second;
			map[uuid] = profile;
			write_map(cfg, kMapKey, map);
			legacy.erase(old);
			write_map(cfg, kLegacyMapKey, legacy);
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
	auto map = read_map(cfg, kMapKey);
	map[scene] = profile;
	write_map(cfg, kMapKey, map);
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

	auto map = read_map(cfg, kMapKey);
	map[uuid] = profile;
	write_map(cfg, kMapKey, map);

	/* Remove the legacy name entry so the same scene does not appear twice. */
	auto legacy = read_map(cfg, kLegacyMapKey);
	if (!name.empty() && legacy.erase(name) > 0)
		write_map(cfg, kLegacyMapKey, legacy);

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
	for (const char *key : {kMapKey, kLegacyMapKey}) {
		auto map = read_map(cfg, key);
		if (map.erase(scene) > 0) {
			write_map(cfg, key, map);
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

	for (const auto &entry : read_map(cfg, kLegacyMapKey))
		display[entry.first] = entry.second;

	const auto names = current_scene_names();
	for (const auto &entry : read_map(cfg, kMapKey)) {
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
	config_remove_value(cfg, kSection, kMapKey);
	config_remove_value(cfg, kSection, kLegacyMapKey);
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

	auto legacy = read_map(cfg, kLegacyMapKey);
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
	auto map = read_map(cfg, kMapKey);
	for (auto it = map.begin(); it != map.end();) {
		if (!looks_like_uuid(it->first) && valid.find(it->first) == valid.end()) {
			it = map.erase(it);
			changed = true;
		} else {
			++it;
		}
	}

	if (changed) {
		write_map(cfg, kLegacyMapKey, legacy);
		write_map(cfg, kMapKey, map);
		blog(LOG_INFO, "[TopScreen] removed scene profile assignments for deleted scenes");
	}
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
	auto map = read_map(cfg, kMapKey);
	bool changed = false;
	for (auto it = map.begin(); it != map.end();) {
		if (looks_like_uuid(it->first) && valid.find(it->first) == valid.end()) {
			it = map.erase(it);
			changed = true;
		} else {
			++it;
		}
	}
	if (changed) {
		write_map(cfg, kMapKey, map);
		blog(LOG_INFO, "[TopScreen] removed scene profile assignments for deleted scenes");
	}
}

} // namespace topscreen::scene_profiles
