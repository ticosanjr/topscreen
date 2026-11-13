#include "ts-filter.hpp"

#include "ts-engine.hpp"

using namespace topscreen;

namespace {

const char *filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FilterName");
}

void *filter_create(obs_data_t *settings, obs_source_t *source)
{
	return ts_engine_create(settings, source, false);
}

void filter_destroy(void *data)
{
	ts_engine_destroy(data);
}

void filter_update(void *data, obs_data_t *settings)
{
	ts_engine_update(data, settings);
}

void filter_defaults(obs_data_t *settings)
{
	ts_engine_defaults(settings);
}

void filter_tick(void *data, float seconds)
{
	ts_engine_tick(data, seconds);
}

void filter_render(void *data, gs_effect_t *effect)
{
	ts_engine_render_filter(data, effect);
}

obs_properties_t *filter_properties(void *data)
{
	return ts_engine_properties(data, nullptr);
}

} // namespace

struct obs_source_info ts_filter_source_info = {
	.id = "topscreen_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = filter_name,
	.create = filter_create,
	.destroy = filter_destroy,
	.get_defaults = filter_defaults,
	.get_properties = filter_properties,
	.update = filter_update,
	.video_tick = filter_tick,
	.video_render = filter_render,
};
