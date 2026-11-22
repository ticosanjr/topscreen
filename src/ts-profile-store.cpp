#include "ts-profile-store.hpp"

#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "ts-settings.hpp"

namespace topscreen::profile_store {

namespace {

using namespace topscreen::core;
using namespace topscreen::core::keys;

constexpr const char *kNameKey = "profile_name";

bool slot_exists(obs_data_t *settings, int index)
{
	if (!settings)
		return false;
	obs_data_t *slot = obs_data_get_obj(settings, kProfileSlots[index]);
	if (!slot)
		return false;
	obs_data_release(slot);
	return true;
}

} // namespace

void seed_slot(obs_data_t *settings, int index)
{
	if (!settings || index < 0 || index >= kProfileCount)
		return;

	obs_data_t *slot = obs_data_create();
	const Config seed = make_profile_seed(index);
	config_store(slot, seed);
	obs_data_set_obj(settings, kProfileSlots[index], slot);
	obs_data_release(slot);
}

bool ensure_slots(obs_data_t *settings, int revision)
{
	if (!settings)
		return false;

	const int current = static_cast<int>(obs_data_get_int(settings, kProfileSeedVersion));
	const bool reseed = current < revision;
	bool changed = false;

	/* A revision bump only seeds the slots that are missing. Existing slots
	 * are left untouched: load_slot() already falls back to the current seed
	 * for any key a slot does not have, so new defaults reach old profiles
	 * without overwriting the user's edits. */
	for (int i = 0; i < kProfileCount; ++i) {
		if (slot_exists(settings, i))
			continue;
		seed_slot(settings, i);
		changed = true;
	}
	if (reseed) {
		obs_data_set_int(settings, kProfileSeedVersion, revision);
		changed = true;
	}
	return changed;
}

std::string load_name(obs_data_t *settings, int index)
{
	if (!settings || index < 0 || index >= kProfileCount)
		return {};
	obs_data_t *slot = obs_data_get_obj(settings, kProfileSlots[index]);
	if (!slot)
		return {};
	const char *name = obs_data_get_string(slot, kNameKey);
	std::string out = name ? name : "";
	obs_data_release(slot);
	return out;
}

void store_name(obs_data_t *settings, int index, const std::string &name)
{
	if (!settings || index < 0 || index >= kProfileCount)
		return;
	obs_data_t *slot = obs_data_get_obj(settings, kProfileSlots[index]);
	if (!slot) {
		seed_slot(settings, index);
		slot = obs_data_get_obj(settings, kProfileSlots[index]);
	}
	if (!slot)
		return;
	obs_data_set_string(slot, kNameKey, name.c_str());
	obs_data_release(slot);
}

std::string display_name(obs_data_t *settings, int index)
{
	const std::string name = load_name(settings, index);
	if (!name.empty())
		return name;
	return core::profile_label(index);
}

void reset_slots(obs_data_t *settings, int revision)
{
	if (!settings)
		return;
	for (int i = 0; i < kProfileCount; ++i)
		seed_slot(settings, i);
	obs_data_set_int(settings, kProfileSeedVersion, revision);
}

Config load_slot(obs_data_t *settings, int index)
{
	const Config seed = make_profile_seed(index);
	if (!settings || index < 0 || index >= kProfileCount)
		return seed;

	obs_data_t *slot = obs_data_get_obj(settings, kProfileSlots[index]);
	if (!slot)
		return seed;

	Config cfg = seed;
	config_load(slot, &cfg, &seed);
	obs_data_release(slot);
	return cfg;
}

void store_slot(obs_data_t *settings, int index, const Config &cfg)
{
	if (!settings || index < 0 || index >= kProfileCount)
		return;

	obs_data_t *slot = obs_data_get_obj(settings, kProfileSlots[index]);
	if (!slot) {
		slot = obs_data_create();
		obs_data_set_obj(settings, kProfileSlots[index], slot);
	}
	config_store(slot, cfg);
	obs_data_release(slot);
}

} // namespace topscreen::profile_store
