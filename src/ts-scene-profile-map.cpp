#include "ts-scene-profiles-internal.hpp"
#include "ts-scene-profiles.hpp"

#include <obs-module.h>
#include <util/config-file.h>

#include <cctype>

#include "core/json.hpp"
#include "core/keys.hpp"

namespace topscreen::scene_profiles::detail {

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

} // namespace topscreen::scene_profiles::detail
