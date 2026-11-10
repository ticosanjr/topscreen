#include "ts-scene-profiles.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>

#include "core/keys.hpp"
#include "core/profiles.hpp"

namespace topscreen::scene_profiles {

int get(const std::string &scene)
{
	if (scene.empty())
		return -1;
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return -1;
	const char *raw = config_get_string(cfg, kSection, scene.c_str());
	if (!raw || !*raw)
		return -1;
	const long long profile = config_get_int(cfg, kSection, scene.c_str());
	if (profile < 0 || profile >= core::keys::kProfileCount)
		return -1;
	return static_cast<int>(profile);
}

bool assign(const std::string &scene, int profile)
{
	if (scene.empty() || profile < 0 || profile >= core::keys::kProfileCount)
		return false;
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return false;
	config_set_int(cfg, kSection, scene.c_str(), profile);
	config_save_safe(cfg, "tmp", nullptr);
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
	config_remove_value(cfg, kSection, scene.c_str());
	config_save_safe(cfg, "tmp", nullptr);
	blog(LOG_INFO, "[TopScreen] scene profile cleared: %s", scene.c_str());
	return true;
}

} // namespace topscreen::scene_profiles
