#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include "ts-filter.hpp"
#include "ts-registry.hpp"

using namespace topscreen;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif
#ifndef TOPSCREEN_SOURCE_DIR
#define TOPSCREEN_SOURCE_DIR "."
#endif

namespace {

/* OBS itself uploads these two in obs_source_process_filter_end(). */
bool obs_owned_uniform(const std::string &name)
{
	return name == "ViewProj" || name == "image";
}

/* Uniforms declared in the effect (float/float2/float4/texture2d). */
std::set<std::string> effect_uniforms()
{
	std::ifstream in(std::string(TOPSCREEN_SOURCE_DIR) + "/data/effects/topscreen.effect");
	std::set<std::string> names;
	std::string line;
	while (std::getline(in, line)) {
		if (line.rfind("uniform ", 0) != 0)
			continue;
		const auto semi = line.find(';');
		if (semi == std::string::npos)
			continue;
		const std::string decl = line.substr(8, semi - 8); /* after "uniform " */
		const auto space = decl.find_last_of(" \t");
		if (space == std::string::npos)
			continue;
		std::string name = decl.substr(space + 1);
		while (!name.empty() && (name.back() == ' ' || name.back() == '\t'))
			name.pop_back();
		if (!name.empty())
			names.insert(name);
	}
	return names;
}

obs_source_t *make_capture(uint32_t w, uint32_t h)
{
	static const obs_source_info info = {
		.id = "fake_capture",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO,
	};
	obs_source_t *s = fakeobs::make_source(&info, nullptr, w, h);
	obs_data_t *set = obs_source_get_settings(s);
	obs_data_set_bool(set, "capture_cursor", true);
	return s;
}

} // namespace

/* Regression: on D3D11 the device refuses to draw when any scalar shader
 * parameter was never uploaded ("Not all shader parameters were set"), which
 * left the source invisible on Windows. Every uniform declared by the effect
 * must therefore be uploaded on a plain frame - with the default config
 * (no wallpaper, no active clicks). */
TEST(render_uploads_every_shader_uniform)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *capture = make_capture(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, capture, 0, 0);

	ts_filter_source_info.get_defaults(filter->settings);
	void *ctx = ts_filter_source_info.create(filter->settings, filter);
	REQUIRE(ctx != nullptr);

	/* A couple of frames with the default config. */
	fakeobs::clear_uniforms();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ts_filter_source_info.video_render(ctx, nullptr);

	const std::set<std::string> declared = effect_uniforms();
	CHECK(!declared.empty());
	CHECK(declared.size() > 30);

	for (const auto &name : declared) {
		if (obs_owned_uniform(name))
			continue;
		++tst::checks();
		const auto &seen = fakeobs::uploaded_uniforms();
		if (std::find(seen.begin(), seen.end(), name) == seen.end())
			tst::fail(__FILE__, __LINE__,
				  "uniform never uploaded (D3D11 aborts the draw): " + name);
	}

	ts_filter_source_info.destroy(ctx);
}

/* The same must hold with the effects disabled and with a background image
 * missing (the states that used to skip uploads). */
TEST(render_uploads_uniforms_with_effects_off)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *capture = make_capture(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, capture, 0, 0);

	ts_filter_source_info.get_defaults(filter->settings);
	void *ctx = ts_filter_source_info.create(filter->settings, filter);
	REQUIRE(ctx != nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	ctrl->set_effects_enabled(false);
	fakeobs::clear_uniforms();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ts_filter_source_info.video_render(ctx, nullptr);

	const auto &seen = fakeobs::uploaded_uniforms();
	for (const auto &name : effect_uniforms()) {
		if (obs_owned_uniform(name))
			continue;
		++tst::checks();
		if (std::find(seen.begin(), seen.end(), name) == seen.end())
			tst::fail(__FILE__, __LINE__, "uniform never uploaded: " + name);
	}

	ts_filter_source_info.destroy(ctx);
}
