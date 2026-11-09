#include "ts-profile-store.hpp"

#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "ts-settings.hpp"

namespace topscreen::profile_store {

namespace {

using namespace topscreen::core;
using namespace topscreen::core::keys;

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

	for (int i = 0; i < kProfileCount; ++i) {
		if (!reseed && slot_exists(settings, i))
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
