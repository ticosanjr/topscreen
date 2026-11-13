#include "capture/ts-capture-source.hpp"
#include "ts-filter.hpp"

#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

TEST(integration_capture_source_lifecycle_and_properties)
{
	obs_source_t *source = fakeobs::make_source(&topscreen::ts_capture_source_info, nullptr, 0, 0);
	obs_data_t *settings = obs_source_get_settings(source);
	REQUIRE(settings != nullptr);
	topscreen::ts_capture_source_info.get_defaults(settings);
	CHECK(obs_data_get_bool(settings, "capture_cursor"));
	CHECK(!obs_data_get_bool(settings, "capture_click_audio"));

	void *ctx = topscreen::ts_capture_source_info.create(settings, source);
	REQUIRE(ctx != nullptr);
	CHECK_EQ(topscreen::ts_capture_source_info.get_width(ctx), 0u);
	CHECK_EQ(topscreen::ts_capture_source_info.get_height(ctx), 0u);

	obs_properties_t *props = topscreen::ts_capture_source_info.get_properties(ctx);
	REQUIRE(props != nullptr);
	CHECK(obs_properties_get(props, "capture_monitor") != nullptr);
	CHECK(obs_properties_get(props, "capture_cursor") != nullptr);
	CHECK(obs_properties_get(props, "capture_click_audio") != nullptr);
	obs_properties_destroy(props);

	/* The filter's parent-cursor lease toggles this property at runtime. */
	obs_data_set_bool(settings, "capture_cursor", false);
	topscreen::ts_capture_source_info.update(ctx, settings);

	topscreen::ts_capture_source_info.destroy(ctx);
	obs_data_release(settings);
}

TEST(integration_capture_source_cursor_lease)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *capture =
		fakeobs::make_source(&topscreen::ts_capture_source_info, nullptr, 1280, 720);
	obs_data_t *capture_settings = obs_source_get_settings(capture);
	topscreen::ts_capture_source_info.get_defaults(capture_settings);
	void *capture_ctx =
		topscreen::ts_capture_source_info.create(capture_settings, capture);
	REQUIRE(capture_ctx != nullptr);

	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, capture, 0, 0);
	obs_data_t *filter_settings = obs_source_get_settings(filter);
	ts_filter_source_info.get_defaults(filter_settings);
	void *filter_ctx = ts_filter_source_info.create(filter_settings, filter);
	REQUIRE(filter_ctx != nullptr);

	/* TopScreen draws its own cursor: the parent source must stop capturing
	 * the native one. */
	ts_filter_source_info.video_render(filter_ctx, nullptr);
	CHECK(!topscreen::native_cursor_enabled(capture));
	/* The scene setting must stay untouched, or a save would persist "off". */
	CHECK(obs_data_get_bool(capture_settings, "capture_cursor"));

	/* Removing the filter restores the native cursor. */
	ts_filter_source_info.destroy(filter_ctx);
	CHECK(topscreen::native_cursor_enabled(capture));

	topscreen::ts_capture_source_info.destroy(capture_ctx);
	obs_data_release(filter_settings);
	obs_data_release(capture_settings);
}
