#include "ts-scene-profiles.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>

#include <map>

#include "core/json.hpp"
#include "core/keys.hpp"
#include "core/profiles.hpp"

namespace topscreen::scene_profiles {

namespace {

constexpr const char *kMapKey = "map";

std::map<std::string, int> read_map(config_t *cfg)
{
	std::map<std::string, int> out;
	if (!cfg)
		return out;
	const char *json = config_get_string(cfg, kSection, kMapKey);
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

void write_map(config_t *cfg, const std::map<std::string, int> &map)
{
	if (!cfg)
		return;
	obs_data_t *data = obs_data_create();
	for (const auto &entry : map)
		obs_data_set_int(data, entry.first.c_str(), entry.second);
	const char *json = obs_data_get_json(data);
	config_set_string(cfg, kSection, kMapKey, json ? json : "{}");
	obs_data_release(data);
	config_save_safe(cfg, "tmp", nullptr);
}

} // namespace

int get(const std::string &scene)
{
	if (scene.empty())
		return -1;
	config_t *cfg = obs_frontend_get_profile_config();
	const auto map = read_map(cfg);
	const auto it = map.find(scene);
	return it == map.end() ? -1 : it->second;
}

bool assign(const std::string &scene, int profile)
{
	if (scene.empty() || profile < 0 || profile >= core::keys::kProfileCount)
		return false;
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return false;
	auto map = read_map(cfg);
	map[scene] = profile;
	write_map(cfg, map);
	blog(LOG_INFO, "[TopScreen] scene profile: %s -> %s", scene.c_str(),
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
	auto map = read_map(cfg);
	if (map.erase(scene) == 0)
		return false;
	write_map(cfg, map);
	blog(LOG_INFO, "[TopScreen] scene profile cleared: %s", scene.c_str());
	return true;
}

std::vector<std::pair<std::string, int>> list()
{
	config_t *cfg = obs_frontend_get_profile_config();
	const auto map = read_map(cfg);
	std::vector<std::pair<std::string, int>> out;
	out.reserve(map.size());
	for (const auto &entry : map)
		out.emplace_back(entry.first, entry.second);
	return out;
}

bool clear_all()
{
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return false;
	config_remove_value(cfg, kSection, kMapKey);
	config_save_safe(cfg, "tmp", nullptr);
	blog(LOG_INFO, "[TopScreen] all scene profile assignments cleared");
	return true;
}

void prune(const std::vector<std::string> &scene_names)
{
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return;
	auto map = read_map(cfg);
	if (map.empty())
		return;

	std::map<std::string, bool> valid;
	for (const auto &name : scene_names)
		valid[name] = true;

	bool changed = false;
	for (auto it = map.begin(); it != map.end();) {
		if (valid.find(it->first) == valid.end()) {
			it = map.erase(it);
			changed = true;
		} else {
			++it;
		}
	}
	if (changed) {
		write_map(cfg, map);
		blog(LOG_INFO, "[TopScreen] removed scene profile assignments for deleted scenes");
	}
}

} // namespace topscreen::scene_profiles
