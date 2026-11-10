#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

#include <algorithm>
#include <string>

#include "webcam/ts-webcam-filter.hpp"

using namespace topscreen;

namespace {

obs_source_t *make_webcam_parent(uint32_t w, uint32_t h)
{
	static const obs_source_info info = {
		.id = "fake_webcam",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO,
	};
	return fakeobs::make_source(&info, nullptr, w, h);
}

bool uploaded(const std::string &name)
{
	const auto &seen = fakeobs::uploaded_uniforms();
	return std::find(seen.begin(), seen.end(), name) != seen.end();
}

} // namespace

TEST(webcam_bubble_filter_uploads_its_uniforms)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_webcam_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_webcam_filter_source_info, parent, 0, 0);
	ts_webcam_filter_source_info.get_defaults(filter->settings);
	void *ctx = ts_webcam_filter_source_info.create(filter->settings, filter);
	REQUIRE(ctx != nullptr);

	obs_properties_t *props = ts_webcam_filter_source_info.get_properties(ctx);
	REQUIRE(props != nullptr);
	CHECK(obs_properties_get(props, "webcam_mask") != nullptr);
	obs_properties_destroy(props);

	fakeobs::clear_uniforms();
	ts_webcam_filter_source_info.video_render(ctx, nullptr);
	CHECK(uploaded("target_size"));
	CHECK(uploaded("bubble_center"));
	CHECK(uploaded("bubble_half"));
	CHECK(uploaded("shape"));
	CHECK(uploaded("border_width"));
	CHECK(uploaded("shadow_blur"));

	ts_webcam_filter_source_info.destroy(ctx);
}
