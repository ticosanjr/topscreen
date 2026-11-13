#include "properties/ts-properties-internal.hpp"

#include "core/keys.hpp"

using namespace topscreen::core;

namespace topscreen::properties {

void set_tip(obs_property_t *prop, const char *description)
{
	if (prop)
		obs_property_set_long_description(prop, description);
}

obs_property_t *add_range_slider(obs_properties_t *props, const char *key, const char *text,
				 const limits::Range &range)
{
	return obs_properties_add_float_slider(props, key, text, range.min, range.max,
					       range.step);
}

obs_property_t *add_range_int_slider(obs_properties_t *props, const char *key, const char *text,
				     const limits::IntRange &range)
{
	return obs_properties_add_int_slider(props, key, text, range.min, range.max, range.step);
}

obs_property_t *add_range_int(obs_properties_t *props, const char *key, const char *text,
			      const limits::IntRange &range)
{
	return obs_properties_add_int(props, key, text, range.min, range.max, range.step);
}

bool setting_bool(obs_data_t *settings, const char *key, bool fallback)
{
	return settings ? obs_data_get_bool(settings, key) : fallback;
}

long long setting_int(obs_data_t *settings, const char *key, long long fallback)
{
	return settings ? obs_data_get_int(settings, key) : fallback;
}

} // namespace topscreen::properties
