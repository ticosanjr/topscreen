#include "capture/ts-capture-source.hpp"

#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

TEST(integration_capture_source_lifecycle_and_properties)
{
	obs_source_t *source = fakeobs::make_source(&topscreen::ts_capture_source_info, nullptr, 0, 0);
	obs_data_t *settings = obs_source_get_settings(source);
	REQUIRE(settings != nullptr);
	topscreen::ts_capture_source_info.get_defaults(settings);

	void *ctx = topscreen::ts_capture_source_info.create(settings, source);
	REQUIRE(ctx != nullptr);
	CHECK_EQ(topscreen::ts_capture_source_info.get_width(ctx), 0u);
	CHECK_EQ(topscreen::ts_capture_source_info.get_height(ctx), 0u);

	obs_properties_t *props = topscreen::ts_capture_source_info.get_properties(ctx);
	REQUIRE(props != nullptr);
	CHECK(obs_properties_get(props, "capture_monitor") != nullptr);
	obs_properties_destroy(props);

	topscreen::ts_capture_source_info.destroy(ctx);
	obs_data_release(settings);
}
