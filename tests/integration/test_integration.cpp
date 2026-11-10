#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <cstdlib>
#include <thread>
#include <string>
#include <vector>

#if defined(_WIN32)
#define TS_SETENV(k, v) _putenv_s(k, v)
#define TS_UNSETENV(k) _putenv_s(k, "")
#else
#define TS_SETENV(k, v) ::setenv(k, v, 1)
#define TS_UNSETENV(k) ::unsetenv(k)
#endif

#include "core/keys.hpp"
#include "core/file.hpp"
#include "core/profiles.hpp"
#include "core/types.hpp"
#include "ts-filter.hpp"
#include "ts-scene-profiles.hpp"
#include "ts-properties.hpp"
#include "ts-profiles-ui.hpp"
#include "render/ts-wallpaper.hpp"
#include "ts-registry.hpp"
#include "hotkeys/ts-hotkeys.hpp"

using namespace topscreen;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

namespace {

/* Fake parent source that exposes the "capture_cursor" property (like a Screen Capture). */
obs_properties_t *parent_props(void *)
{
	obs_properties_t *p = obs_properties_create();
	obs_properties_add_bool(p, "capture_cursor", "Capture cursor");
	return p;
}

const obs_source_info parent_info = {
	.id = "fake_display_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_properties = parent_props,
};

obs_source_t *make_parent(uint32_t w, uint32_t h)
{
	obs_source_t *parent = fakeobs::make_source(&parent_info, nullptr, w, h);
	obs_data_set_bool(parent->settings, "capture_cursor", true);
	return parent;
}

void *create_filter(obs_source_t *filter)
{
	/* In real OBS, the obs_data passed to create IS the source's own. */
	ts_filter_source_info.get_defaults(filter->settings);
	void *ctx = ts_filter_source_info.create(filter->settings, filter);
	fakeobs::attach_filter_ctx(filter, ctx);
	return ctx;
}

void destroy_filter(void *ctx)
{
	ts_filter_source_info.destroy(ctx);
	fakeobs::detach_filter_ctx(ctx);
}

/* Simulates the profile list in Properties: writes profile_current and calls update. */
void set_profile_from_properties(obs_source_t *filter, void *ctx, int index)
{
	obs_data_set_int(filter->settings, core::keys::kProfileCurrent, index);
	ts_filter_source_info.update(ctx, filter->settings);
}

bool slot_effects_enabled(obs_source_t *filter, int index)
{
	obs_data_t *slot = obs_data_get_obj(filter->settings, core::keys::kProfileSlots[index]);
	if (!slot)
		return false;
	const bool value = obs_data_get_bool(slot, core::keys::kEffectsEnabled);
	obs_data_release(slot);
	return value;
}

} // namespace

TEST(integration_filter_lifecycle)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	CHECK_EQ(ctrl->preset_count(), 3);

	/* Render sets the target dimensions and, if the effect loaded, disables
	 * the parent's native cursor (T2.1). */
	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));

	for (int i = 0; i < 10; ++i)
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	REQUIRE(fakeobs::hotkey_count() >= 12);

	/* Toggle effects via hotkey. */
	const int toggle = fakeobs::find_hotkey("TopScreen_ToggleEffects");
	REQUIRE(toggle >= 0);
	fakeobs::invoke_hotkey(toggle, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->effects_enabled());
	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(obs_data_get_bool(parent->settings, "capture_cursor"));

	/* Zoom via hotkey must not crash anything. */
	const int zin = fakeobs::find_hotkey("TopScreen_ZoomIn");
	REQUIRE(zin >= 0);
	fakeobs::invoke_hotkey(zin, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Preset via FilterControl (UI thread -> applied on tick). */
	ctrl->apply_preset(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_preset(), 1);
	/* tutorial.json uses a solid background. The modern key is written, and
	 * the legacy alias keeps old readers working. */
	CHECK_EQ(static_cast<int>(obs_data_get_int(filter->settings, core::keys::kBgType)), 0);
	CHECK_EQ(static_cast<int>(obs_data_get_int(filter->settings, core::keys::kBgMode)), 0);

	destroy_filter(ctx);
	CHECK(registry_first() == nullptr);
}

TEST(integration_all_presets_and_toggle_background)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	for (int i = 0; i < ctrl->preset_count(); ++i) {
		ctrl->apply_preset(i);
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		CHECK_EQ(ctrl->current_preset(), i);
		CHECK(!ctrl->preset_name(i).empty());
	}

	const int tg = fakeobs::find_hotkey("TopScreen_ToggleBackground");
	REQUIRE(tg >= 0);
	const bool before = obs_data_get_bool(filter->settings, core::keys::kBgEnabled);
	fakeobs::invoke_hotkey(tg, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!obs_data_get_bool(filter->settings, core::keys::kBgEnabled) != !before);

	destroy_filter(ctx);
}

TEST(integration_hotkeys_all_actions)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	const char *names[] = {"TopScreen_ToggleEffects", "TopScreen_ToggleBackground",
			       "TopScreen_ZoomIn",	      "TopScreen_ZoomOut",
			       "TopScreen_ResetZoom",	      "TopScreen_NextPreset",
			       "TopScreen_PrevPreset",	      "TopScreen_ProfileA",
			       "TopScreen_ProfileB",	      "TopScreen_ProfileC",
			       "TopScreen_ProfileD",	      "TopScreen_ProfileE",
			       "TopScreen_ProfileF",	      "TopScreen_ProfileG",
			       "TopScreen_ProfileH",	      "TopScreen_ProfileI",
			       "TopScreen_ProfileJ",	      "TopScreen_ProfileSave",
			       "TopScreen_ToggleHighlight",
			       "TopScreen_ToggleSpotlight", "TopScreen_ToggleMagnifier",
			       "TopScreen_ToggleCursor"};
	for (const char *n : names)
		REQUIRE(fakeobs::find_hotkey(n) >= 0);

	/* pressed=false does nothing (callback early-return). */
	const bool fx0 = ctrl->effects_enabled();
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_ToggleEffects"), false);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_ProfileC"), false);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->effects_enabled() == fx0);
	CHECK_EQ(ctrl->current_profile(), 0);

	/* pressed=true: each action reaches the filter. */
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_ToggleEffects"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->effects_enabled() == !fx0);

	const bool bg0 = obs_data_get_bool(filter->settings, core::keys::kBgEnabled);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_ToggleBackground"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_bool(filter->settings, core::keys::kBgEnabled) == !bg0);

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_ZoomIn"), true);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_ZoomOut"), true);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_ResetZoom"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_NextPreset"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->current_preset() >= 0 && ctrl->current_preset() < ctrl->preset_count());
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_PrevPreset"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->current_preset() >= 0 && ctrl->current_preset() < ctrl->preset_count());

	const char *profiles[] = {"TopScreen_ProfileA", "TopScreen_ProfileB", "TopScreen_ProfileC",
				   "TopScreen_ProfileD", "TopScreen_ProfileE", "TopScreen_ProfileF",
				   "TopScreen_ProfileG", "TopScreen_ProfileH", "TopScreen_ProfileI",
				   "TopScreen_ProfileJ"};
	for (int i = 0; i < 10; ++i) {
		fakeobs::invoke_hotkey(fakeobs::find_hotkey(profiles[i]), true);
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		CHECK_EQ(ctrl->current_profile(), i);
	}

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen_ProfileSave"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	obs_data_t *saved_slot =
		obs_data_get_obj(filter->settings, core::keys::kProfileSlots[ctrl->current_profile()]);
	CHECK(saved_slot != nullptr);
	obs_data_release(saved_slot);

	destroy_filter(ctx);
}

TEST(integration_key_overlay)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	/* Injects keys through the debug hook (without an input back-end). */
	TS_SETENV("TOPSCREEN_DEBUG_KEY", "Ctrl");

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	ts_filter_source_info.video_render(ctx, nullptr);

	/* Runs enough frames for the hook to fire and the overlay to be drawn. */
	for (int i = 0; i < 130; ++i) {
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		ts_filter_source_info.video_render(ctx, nullptr);
	}

	TS_UNSETENV("TOPSCREEN_DEBUG_KEY");

	destroy_filter(ctx);
	CHECK(registry_first() == nullptr);
}

TEST(integration_profiles_save_and_switch)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	CHECK_EQ(ctrl->profile_count(), 10);

	/* Profile A: current state (effects on) -> save. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_profile(), 0);
	CHECK(ctrl->effects_enabled());
	ctrl->save_profile();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Profile B: effects off -> save. */
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->set_effects_enabled(false);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->effects_enabled());
	ctrl->save_profile();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Back to A: effects on again. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_profile(), 0);
	CHECK(ctrl->effects_enabled());

	/* And to B: off. */
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_profile(), 1);
	CHECK(!ctrl->effects_enabled());

	/* Out-of-range profile must not break and must stay in range. */
	ctrl->select_profile(ctrl->profile_count() + 5);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->current_profile() >= 0 && ctrl->current_profile() < ctrl->profile_count());

	destroy_filter(ctx);
	CHECK(registry_first() == nullptr);
}

/* Creating the filter must register every hotkey on the source, once each,
 * with the stable TopScreen_* ids (the OBS UI groups them under the filter
 * instance name, so a missing section means these never registered). */
/* Profiles backup: requests are queued to the video thread and invalid files
 * are rejected before touching the source (the fake runtime reports the file
 * write as failed, which exercises the result path too). */
TEST(integration_profiles_io_roundtrip)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	CHECK_EQ(ctrl->profile_io_result(), 0);

	/* A = effects off, B = effects on. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->set_effects_enabled(false);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->save_profile();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->set_effects_enabled(true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->save_profile();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Invalid import is rejected before touching anything. */
	ctrl->import_profiles("{\"not\":\"a bundle\"}");
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->profile_io_result(), 4);
	CHECK_EQ(ctrl->current_profile(), 1);
	CHECK(ctrl->effects_enabled());

	/* Export while profile A is active. */
	const std::string path =
		(std::filesystem::temp_directory_path() / "topscreen-profiles-test.json").string();
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->export_profiles(path);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->profile_io_result(), 1);

	/* Overwrite both slots with the opposite values. */
	ctrl->set_effects_enabled(true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->save_profile();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->set_effects_enabled(false);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->save_profile();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Leave a pending debounced edit: importing must discard it instead of
	 * writing it over the imported profile after the debounce expires. */
	ctrl->set_effects_enabled(true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->effects_enabled());

	/* Import the file text (the dock validates it and passes the text). */
	std::string text;
	REQUIRE(core::read_file_text(path.c_str(), &text));
	ctrl->import_profiles(text);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->profile_io_result(), 3);
	CHECK_EQ(ctrl->current_profile(), 0);
	CHECK(!ctrl->effects_enabled()); /* A: off restored */

	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->effects_enabled()); /* B: on restored */

	std::remove(path.c_str());
	destroy_filter(ctx);
}

TEST(integration_registry_target_selection)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *f1 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	obs_source_t *f2 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	fakeobs::set_source_name(f1, "TopScreen A");
	fakeobs::set_source_name(f2, "TopScreen B");

	void *ctx1 = create_filter(f1);
	void *ctx2 = create_filter(f2);
	REQUIRE(ctx1 != nullptr);
	REQUIRE(ctx2 != nullptr);

	const std::uint64_t revision = registry_revision();
	CHECK(revision > 0);

	const std::vector<FilterEntry> entries = registry_list();
	REQUIRE(entries.size() == 2);
	CHECK_EQ(entries[0].name, std::string("TopScreen A"));
	CHECK_EQ(entries[1].name, std::string("TopScreen B"));
	FilterControl *a = entries[0].control;
	FilterControl *b = entries[1].control;
	CHECK(registry_first() == a); // first registered is the default target

	/* The dock selection also routes hotkeys to that filter. */
	REQUIRE(registry_select(b));
	CHECK(registry_first() == b);
	CHECK(dynamic_cast<FilterControl *>(hotkey_target_first()) == b);

	const int toggle = fakeobs::find_hotkey("TopScreen_ToggleEffects");
	REQUIRE(toggle >= 0);
	fakeobs::invoke_hotkey(toggle, true);
	ts_filter_source_info.video_tick(ctx2, 1.0f / 60.0f);
	CHECK(!b->effects_enabled());

	ts_filter_source_info.video_tick(ctx1, 1.0f / 60.0f);
	CHECK(a->effects_enabled());

	/* Deleting the selected filter falls back to the remaining one. */
	destroy_filter(ctx2);
	CHECK(registry_first() == a);
	CHECK(dynamic_cast<FilterControl *>(hotkey_target_first()) == a);

	destroy_filter(ctx1);
	CHECK(registry_first() == nullptr);
	CHECK(hotkey_target_first() == nullptr);
}

TEST(integration_profile_autosave_is_debounced)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	const bool before = slot_effects_enabled(filter, 0);
	const bool target = !before;

	/* A dock/hotkey edit updates the live config immediately, but the profile
	 * slot is not written on the same tick. */
	ctrl->set_effects_enabled(target);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->effects_enabled(), target);
	CHECK_EQ(slot_effects_enabled(filter, 0), before);

	/* 400 ms after the last edit, the debounce flushes the slot. */
	ts_filter_source_info.video_tick(ctx, 0.5f);
	CHECK_EQ(slot_effects_enabled(filter, 0), target);

	/* Switching profiles before the debounce expires flushes the edit to the
	 * profile being left instead of losing it. */
	ctrl->set_effects_enabled(before);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(slot_effects_enabled(filter, 0), target);

	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(slot_effects_enabled(filter, 0), before);
	CHECK_EQ(ctrl->current_profile(), 1);

	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_profile(), 0);
	CHECK_EQ(ctrl->effects_enabled(), before);

	destroy_filter(ctx);
}

TEST(integration_monitor_origin_from_environment)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	/* Monitor below the primary one: global y starts at 1080. */
	TS_SETENV("TOPSCREEN_MONITOR_ORIGIN", "0,1080");

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	double x = -1.0, y = -1.0;
	ctrl->monitor_origin(&x, &y);
	CHECK_NEAR(x, 0.0, 1e-9);
	CHECK_NEAR(y, 1080.0, 1e-9);

	destroy_filter(ctx);

	/* Without the override and without a resolvable monitor list, the origin
	 * stays at (0,0): the previous behaviour is preserved. */
	TS_UNSETENV("TOPSCREEN_MONITOR_ORIGIN");
	obs_source_t *filter2 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx2 = create_filter(filter2);
	REQUIRE(ctx2 != nullptr);
	ts_filter_source_info.video_render(ctx2, nullptr);
	ts_filter_source_info.video_tick(ctx2, 1.0f / 60.0f);

	FilterControl *ctrl2 = registry_first();
	REQUIRE(ctrl2 != nullptr);
	double x2 = -1.0, y2 = -1.0;
	ctrl2->monitor_origin(&x2, &y2);
	CHECK_NEAR(x2, 0.0, 1e-9);
	CHECK_NEAR(y2, 0.0, 1e-9);

	destroy_filter(ctx2);
}

TEST(integration_monitor_origin_manual_overrides_environment)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	/* Environment says (0,1080); the manual fields must win. */
	TS_SETENV("TOPSCREEN_MONITOR_ORIGIN", "0,1080");

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	obs_data_t *set = filter->settings;
	ts_filter_source_info.get_defaults(set);
	obs_data_set_bool(set, core::keys::kMonitorManual, true);
	obs_data_set_int(set, core::keys::kMonitorX, -1920);
	obs_data_set_int(set, core::keys::kMonitorY, 0);

	void *ctx = ts_filter_source_info.create(set, filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	double x = 0.0, y = 0.0;
	ctrl->monitor_origin(&x, &y);
	CHECK_NEAR(x, -1920.0, 1e-9);
	CHECK_NEAR(y, 0.0, 1e-9);

	destroy_filter(ctx);
	TS_UNSETENV("TOPSCREEN_MONITOR_ORIGIN");
}

TEST(integration_registers_all_hotkeys)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	const int before = fakeobs::hotkey_count();
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	const char *names[] = {"TopScreen_ToggleEffects", "TopScreen_ToggleBackground",
			       "TopScreen_ZoomIn",	      "TopScreen_ZoomOut",
			       "TopScreen_ResetZoom",	      "TopScreen_NextPreset",
			       "TopScreen_PrevPreset",	      "TopScreen_ProfileA",
			       "TopScreen_ProfileB",	      "TopScreen_ProfileC",
			       "TopScreen_ProfileD",	      "TopScreen_ProfileE",
			       "TopScreen_ProfileF",	      "TopScreen_ProfileG",
			       "TopScreen_ProfileH",	      "TopScreen_ProfileI",
			       "TopScreen_ProfileJ",	      "TopScreen_ProfileSave",
			       "TopScreen_ToggleHighlight",
			       "TopScreen_ToggleSpotlight", "TopScreen_ToggleMagnifier",
			       "TopScreen_ToggleCursor"};
	const int expected = static_cast<int>(sizeof(names) / sizeof(names[0]));

	/* The harness keeps earlier registrations, so measure this filter's delta. */
	CHECK_EQ(fakeobs::hotkey_count() - before, expected);
	for (const char *n : names)
		CHECK(fakeobs::find_hotkey(n) >= 0);

	/* Every callback must be wired and active (a null func would make the UI
	 * list an entry that never fires). */
	CHECK(fakeobs::hotkeys_active_from(before));

	destroy_filter(ctx);
	CHECK(!fakeobs::hotkeys_active_from(before));
}

/* Frontend hotkeys are process-wide: a second filter must NOT register a
 * second set of 22, and destroying one filter must keep them alive. */
TEST(integration_hotkeys_shared_between_filters)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *f1 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	obs_source_t *f2 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	const int before = fakeobs::hotkey_count();

	void *ctx1 = create_filter(f1);
	void *ctx2 = create_filter(f2);
	REQUIRE(ctx1 != nullptr);
	REQUIRE(ctx2 != nullptr);

	CHECK_EQ(fakeobs::hotkey_count() - before, 22);
	CHECK(fakeobs::hotkeys_active_from(before));

	/* Destroying one filter keeps the shared hotkeys working. */
	destroy_filter(ctx1);
	CHECK(fakeobs::hotkeys_active_from(before));

	destroy_filter(ctx2);
	CHECK(!fakeobs::hotkeys_active_from(before));
}

TEST(integration_seeded_profiles_restore)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	/* Profile A (balanced): zoom 1.35 and white left click. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.35, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFFFFFF));

	/* Profile B (clean): transparent background and zoom off. */
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kZoomMode),
		 static_cast<long long>(core::ZoomMode::Off));
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kBgType),
		 static_cast<long long>(core::BgMode::Transparent));

	/* Going back to A must restore EVERYTHING. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.35, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFFFFFF));

	destroy_filter(ctx);
}

TEST(integration_profile_from_list_restores_everything)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Profile B (seeded): transparent background + zoom off. */
	set_profile_from_properties(filter, ctx, 1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kZoomMode),
		 static_cast<long long>(core::ZoomMode::Off));
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kBgType),
		 static_cast<long long>(core::BgMode::Transparent));

	/* The main keys must come along, otherwise the next reload undoes the profile. */
	CHECK(obs_data_has_user_value(filter->settings, core::keys::kZoomMode));

	/* Changing ANOTHER property must not undo the active profile. */
	obs_data_set_bool(filter->settings, core::keys::kHighlightEnabled, false);
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kZoomMode),
		 static_cast<long long>(core::ZoomMode::Off));
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kBgType),
		 static_cast<long long>(core::BgMode::Transparent));

	/* Profile C (seeded): presentation, zoom 1.25 + keys on. */
	set_profile_from_properties(filter, ctx, 2);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.25, 1e-6);
	CHECK(obs_data_get_bool(filter->settings, core::keys::kKeysEnabled));

	destroy_filter(ctx);
	CHECK(registry_first() == nullptr);
}

/* Profile/preset changes replace many widgets at once. OBS does not refresh
 * the Filters dialog on obs_source_update(), so the plugin must emit
 * update_properties or the cursor/wallpaper/sound dropdowns show stale values
 * from the profile that was active before. */
TEST(integration_profile_switch_refreshes_properties)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	/* Dock/hotkey path. */
	fakeobs::reset_property_update_calls();
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(fakeobs::property_update_calls() > 0);

	/* Properties profile-list path. */
	fakeobs::reset_property_update_calls();
	set_profile_from_properties(filter, ctx, 2);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(fakeobs::property_update_calls() > 0);

	/* Preset changes also replace multiple assets/widgets. */
	fakeobs::reset_property_update_calls();
	ctrl->apply_preset(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(fakeobs::property_update_calls() > 0);

	destroy_filter(ctx);
}

TEST(integration_hotkey_highlight_toggles_zoom)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	CHECK(!ctrl->hotkey_highlight_active());

	ctrl->toggle_highlight();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->hotkey_highlight_active());

	/* First press stays zoomed while held (it is a toggle, not a timed hold). */
	for (int i = 0; i < 130; ++i)
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->hotkey_highlight_active());

	/* A manual zoom takes over the same zoom model and clears the toggle. */
	const int zin = fakeobs::find_hotkey("TopScreen_ZoomIn");
	REQUIRE(zin >= 0);
	fakeobs::invoke_hotkey(zin, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->hotkey_highlight_active());

	/* Next press starts the hotkey highlight again, next press releases it. */
	ctrl->toggle_highlight();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->hotkey_highlight_active());
	ctrl->toggle_highlight();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->hotkey_highlight_active());

	/* The toggle is independent of the drag master switch. */
	obs_data_set_bool(filter->settings, core::keys::kHighlightEnabled, false);
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->toggle_highlight();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->hotkey_highlight_active());
	ctrl->toggle_highlight();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->hotkey_highlight_active());

	/* Disabling the independent hotkey mode releases an active gesture. */
	ctrl->toggle_highlight();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->hotkey_highlight_active());
	obs_data_set_bool(filter->settings, core::keys::kHighlightHotkeyEnabled, false);
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->hotkey_highlight_active());

	destroy_filter(ctx);
}

TEST(integration_hotkey_toggles_spotlight)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	const int toggle = fakeobs::find_hotkey("TopScreen_ToggleSpotlight");
	REQUIRE(toggle >= 0);
	CHECK(!obs_data_get_bool(filter->settings, core::keys::kSpotlightEnabled));

	fakeobs::invoke_hotkey(toggle, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_bool(filter->settings, core::keys::kSpotlightEnabled));

	fakeobs::invoke_hotkey(toggle, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!obs_data_get_bool(filter->settings, core::keys::kSpotlightEnabled));

	destroy_filter(ctx);
}

TEST(integration_hotkey_toggles_magnifier_and_blocks_spotlight)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	const int spotlight = fakeobs::find_hotkey("TopScreen_ToggleSpotlight");
	const int magnifier = fakeobs::find_hotkey("TopScreen_ToggleMagnifier");
	REQUIRE(spotlight >= 0);
	REQUIRE(magnifier >= 0);

	fakeobs::invoke_hotkey(spotlight, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_bool(filter->settings, core::keys::kSpotlightEnabled));

	fakeobs::invoke_hotkey(magnifier, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_bool(filter->settings, core::keys::kMagnifierEnabled));
	CHECK(!obs_data_get_bool(filter->settings, core::keys::kSpotlightEnabled));

	fakeobs::invoke_hotkey(spotlight, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_bool(filter->settings, core::keys::kSpotlightEnabled));
	CHECK(!obs_data_get_bool(filter->settings, core::keys::kMagnifierEnabled));

	destroy_filter(ctx);
}

TEST(integration_hotkey_highlight_ignored_with_effects_off)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	ctrl->set_effects_enabled(false);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->toggle_highlight();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->hotkey_highlight_active());

	destroy_filter(ctx);
}

TEST(integration_profile_switch_reloads_click_sounds)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	CHECK(ctrl->loaded_sound_path(false).ends_with("sounds/click-soft.wav"));

	/* User report: profile D did not switch the click sound. Give it two
	 * distinct WAVs through the same slot path the Properties editor uses. */
	obs_data_t *slot = obs_data_get_obj(filter->settings, core::keys::kProfileSlots[3]);
	REQUIRE(slot != nullptr);
	obs_data_set_string(slot, core::keys::kClickSoundFileLeft,
			    "sounds/click-hard.wav");
	obs_data_set_string(slot, core::keys::kClickSoundFileRight,
			    "sounds/click-bubble.wav");
	obs_data_release(slot);

	fakeobs::reset_property_update_calls();
	ctrl->select_profile(3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(fakeobs::property_update_calls() > 0);
	CHECK(ctrl->loaded_sound_path(false).ends_with("sounds/click-hard.wav"));
	CHECK(ctrl->loaded_sound_path(true).ends_with("sounds/click-bubble.wav"));

	/* Switch away and back: the reload must not be cached across profiles. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->loaded_sound_path(false).ends_with("sounds/click-soft.wav"));

	ctrl->select_profile(3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->loaded_sound_path(false).ends_with("sounds/click-hard.wav"));
	CHECK(ctrl->loaded_sound_path(true).ends_with("sounds/click-bubble.wav"));

	destroy_filter(ctx);
}

TEST(integration_profile_partial_slot_uses_seed)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Legacy/partial slot: only the wallpaper field exists. */
	obs_data_t *partial = obs_data_create();
	obs_data_set_string(partial, core::keys::kBgImage, "wallpapers/glacier.jpg");
	obs_data_set_obj(filter->settings, core::keys::kProfileSlots[3], partial);
	obs_data_release(partial);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	ctrl->select_profile(3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	CHECK_EQ(obs_data_get_string(filter->settings, core::keys::kBgImage),
		 std::string("wallpapers/glacier.jpg"));
	/* Profile D's seed: auto zoom and gold left click. */
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kZoomMode),
		 static_cast<long long>(core::ZoomMode::Auto));
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00D7FF));

	destroy_filter(ctx);
}

TEST(integration_profile_import_fills_missing_fields)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	/* Deliberately old/partial bundle: one slot, only two fields. */
	const char *json = R"({
		"topscreen_profiles": 1,
		"plugin": "TopScreen",
		"current": 3,
		"profile_slot_3": {
			"cursor_style": 60,
			"bg_image": "wallpapers/glacier.jpg"
		}
	})";
	ctrl->import_profiles(json);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kProfileCurrent), 3);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kCursorStyle), 60);
	CHECK_EQ(obs_data_get_string(filter->settings, core::keys::kBgImage),
		 std::string("wallpapers/glacier.jpg"));
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kBgPaddingX), 36.0, 1e-9);
	CHECK(!obs_data_get_bool(filter->settings, core::keys::kKeysShowSpecial));
	CHECK(obs_data_get_bool(filter->settings, core::keys::kKeysShowShortcuts));
	CHECK_EQ(obs_data_get_string(filter->settings, core::keys::kClickSoundFileLeft),
		 std::string("sounds/click-soft.wav"));
	/* Fields absent from the partial slot keep profile D's seed, not zeros. */
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kZoomMode),
		 static_cast<long long>(core::ZoomMode::Auto));
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00D7FF));

	destroy_filter(ctx);
}

TEST(integration_profile_keeps_user_edit)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Goes to profile D (gold left click). */
	set_profile_from_properties(filter, ctx, 3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00D7FF));

	/* User changes the left click color (a value that exists in no seed). */
	obs_data_set_int(filter->settings, core::keys::kClickLeftColor,
			 static_cast<long long>(0xFF112233));
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Switches to A and back to D: the edit must have stayed in profile D. */
	set_profile_from_properties(filter, ctx, 0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFFFFFF));

	set_profile_from_properties(filter, ctx, 3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF112233));

	destroy_filter(ctx);
}

TEST(integration_scene_profiles_switch_on_scene_change)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	scene_profiles::clear("Scene A");

	fakeobs::set_current_scene_name("Scene A");
	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	CHECK(scene_profiles::assign("Scene A", 4));
	CHECK_EQ(scene_profiles::get("Scene A"), 4);

	fakeobs::fire_frontend_event(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_profile(), 4);

	scene_profiles::clear("Scene A");
	CHECK_EQ(scene_profiles::get("Scene A"), -1);
	fakeobs::set_current_scene_name("Scene");
	destroy_filter(ctx);
}

TEST(integration_chosen_profile_survives_restart)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Chooses profile B and "closes OBS". */
	set_profile_from_properties(filter, ctx, 1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	destroy_filter(ctx);

	/* "Reopens OBS": the saved collection has profile_current + the slots. */
	const std::string saved = obs_data_get_json(filter->settings);
	obs_source_t *f2 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	obs_data_release(f2->settings);
	f2->settings = obs_data_create_from_json(saved.c_str());
	REQUIRE(f2->settings != nullptr);
	void *ctx2 = create_filter(f2);
	REQUIRE(ctx2 != nullptr);
	ts_filter_source_info.video_tick(ctx2, 1.0f / 60.0f);

	/* The state must be that of profile B (not the default). */
	CHECK_EQ(obs_data_get_int(f2->settings, core::keys::kZoomMode),
		 static_cast<long long>(core::ZoomMode::Off));
	CHECK_EQ(obs_data_get_int(f2->settings, core::keys::kBgType),
		 static_cast<long long>(core::BgMode::Transparent));

	destroy_filter(ctx2);
}

TEST(integration_render_does_not_crash_without_effect)
{
	fakeobs::set_data_dir("/diretorio/inexistente");

	obs_source_t *parent = make_parent(640, 480);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	/* Without effect (invalid data dir) => passthrough, no crash. */
	for (int i = 0; i < 5; ++i) {
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		ts_filter_source_info.video_render(ctx, nullptr);
	}

	/* Did not try to touch the parent's cursor (render exited early). */
	CHECK(obs_data_get_bool(parent->settings, "capture_cursor"));

	destroy_filter(ctx);
}

TEST(integration_restores_parent_cursor_on_destroy)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));

	destroy_filter(ctx);
	CHECK(obs_data_get_bool(parent->settings, "capture_cursor"));
}

TEST(integration_keeps_parent_cursor_off_if_it_was_off)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1920, 1080);
	obs_data_set_bool(parent->settings, "capture_cursor", false);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));
	destroy_filter(ctx);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));
}

TEST(integration_native_cursor_follows_cursor_setting)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));

	/* Effects on, but TopScreen cursor disabled: no own cursor, so restore the
	 * native one instead of leaving the screen without any cursor. */
	obs_data_set_bool(filter->settings, core::keys::kCursorEnabled, false);
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(obs_data_get_bool(parent->settings, "capture_cursor"));

	obs_data_set_bool(filter->settings, core::keys::kCursorEnabled, true);
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));

	destroy_filter(ctx);
	CHECK(obs_data_get_bool(parent->settings, "capture_cursor"));
}

TEST(integration_shared_parent_cursor_refcount)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *f1 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	obs_source_t *f2 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx1 = create_filter(f1);
	void *ctx2 = create_filter(f2);
	REQUIRE(ctx1 != nullptr);
	REQUIRE(ctx2 != nullptr);

	ts_filter_source_info.video_render(ctx1, nullptr);
	ts_filter_source_info.video_render(ctx2, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));

	/* The first filter must not restore the cursor while the second still
	 * draws its own. */
	destroy_filter(ctx1);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));

	destroy_filter(ctx2);
	CHECK(obs_data_get_bool(parent->settings, "capture_cursor"));
}

/* Fake parent source that uses "show_cursor" (like Linux's xshm_input). */
namespace {
obs_properties_t *parent_props_show_cursor(void *)
{
	obs_properties_t *p = obs_properties_create();
	obs_properties_add_bool(p, "show_cursor", "Show cursor");
	return p;
}
const obs_source_info parent_info2 = {
	.id = "fake_xshm_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_properties = parent_props_show_cursor,
};
} // namespace

TEST(integration_disables_parent_show_cursor)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = fakeobs::make_source(&parent_info2, nullptr, 1920, 1080);
	obs_data_set_bool(parent->settings, "show_cursor", true);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "show_cursor"));
	destroy_filter(ctx);
	CHECK(obs_data_get_bool(parent->settings, "show_cursor"));
}

TEST(integration_wallpaper_prefilter_stress)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	obs_data_set_bool(filter->settings, core::keys::kEffectsEnabled, true);
	obs_data_set_bool(filter->settings, core::keys::kBgEnabled, true);
	obs_data_set_int(filter->settings, core::keys::kBgType,
			 static_cast<int>(core::BgMode::Wallpaper));
	obs_data_set_int(filter->settings, core::keys::kBgMode,
			 static_cast<int>(core::BgMode::Wallpaper));
	obs_data_set_string(filter->settings, core::keys::kBgImage, "wallpapers/fiber.jpg");
	obs_data_set_double(filter->settings, core::keys::kBgImageBlur, 30.0);
	ts_filter_source_info.update(ctx, filter->settings);

	/* Rapid setting changes while the shared cache worker is decoding and
	 * filtering. This exercises replacement jobs, texture swaps and teardown
	 * with a pending result. */
	for (int i = 0; i < 120; ++i) {
		if (i % 20 == 0) {
			obs_data_set_double(filter->settings, core::keys::kBgImageBlur,
					    static_cast<double>((i * 17) % 61));
			ts_filter_source_info.update(ctx, filter->settings);
		}
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		ts_filter_source_info.video_render(ctx, nullptr);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	CHECK_EQ(obs_data_get_string(filter->settings, core::keys::kBgImage),
		 std::string("wallpapers/fiber.jpg"));
	destroy_filter(ctx);
}

TEST(integration_properties_parent_groups)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	obs_properties_t *props = properties_create(ctx, filter->settings);
	REQUIRE(props != nullptr);
	for (const char *group : {"grp_capture", "grp_pointer", "grp_focus", "grp_overlays",
				  "grp_advanced"})
		CHECK(obs_properties_get(props, group) != nullptr);
	for (const char *child : {"grp_bg", "grp_zoom", "grp_cursor", "grp_click_left",
				  "grp_sound", "grp_spotlight", "grp_magnifier", "grp_keys",
				  "grp_highlight", "grp_monitor"})
		CHECK(obs_properties_get(props, child) != nullptr);

	/* Solid background hides the nested wallpaper group. */
	CHECK(!fakeobs::property_visible(obs_properties_get(props, "grp_bg_image")));

	/* Selecting a wallpaper must switch the background mode, not only store
	 * the path. This is the regression that left the frame on a solid color. */
	obs_property_t *wallpaper = obs_properties_get(props, core::keys::kBgImage);
	REQUIRE(wallpaper != nullptr);
	REQUIRE(wallpaper->modified != nullptr);
	obs_data_set_string(filter->settings, core::keys::kBgImage, "wallpapers/fiber.jpg");
	wallpaper->modified(props, wallpaper, filter->settings);
	CHECK_EQ(static_cast<int>(obs_data_get_int(filter->settings, core::keys::kBgType)),
		 static_cast<int>(core::BgMode::Wallpaper));
	CHECK(fakeobs::property_visible(obs_properties_get(props, "grp_bg_image")));
	obs_properties_destroy(props);

	obs_data_set_int(filter->settings, core::keys::kBgType,
			 static_cast<int>(core::BgMode::Wallpaper));
	props = properties_create(ctx, filter->settings);
	REQUIRE(props != nullptr);
	CHECK(fakeobs::property_visible(obs_properties_get(props, "grp_bg_image")));
	obs_properties_destroy(props);
	destroy_filter(ctx);
}

TEST(integration_wallpaper_pipeline_polls_ready_texture)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	WallpaperPipeline pipeline;
	pipeline.update("wallpapers/fiber.jpg", 1280, 720, 30.0);
	for (int i = 0; i < 3000 && !pipeline.has_texture(); ++i) {
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		pipeline.update("wallpapers/fiber.jpg", 1280, 720, 30.0);
	}
	CHECK(pipeline.has_texture());
	CHECK(pipeline.aspect() > 1.0f);

	/* Switching to another wallpaper must replace the texture without a
	 * detour through Solid mode. */
	pipeline.update("wallpapers/glacier.jpg", 1280, 720, 30.0);
	for (int i = 0; i < 3000 && !pipeline.has_texture(); ++i) {
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		pipeline.update("wallpapers/glacier.jpg", 1280, 720, 30.0);
	}
	CHECK(pipeline.has_texture());
	pipeline.release();
}


/* The new A..J seeds overwrite existing slots once (revision 1 -> 2), then
 * preserve later user edits. */
TEST(integration_profile_seed_revision_overwrites_once)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	obs_data_set_int(filter->settings, core::keys::kProfileSeedVersion, 1);
	obs_data_t *slot = obs_data_create();
	obs_data_set_int(slot, core::keys::kClickLeftColor, 0x112233);
	obs_data_set_obj(filter->settings, core::keys::kProfileSlots[0], slot);
	obs_data_release(slot);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	obs_data_t *seed_slot = obs_data_get_obj(filter->settings, core::keys::kProfileSlots[0]);
	REQUIRE(seed_slot != nullptr);
	CHECK_EQ(obs_data_get_int(seed_slot, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFFFFFF));
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kProfileSeedVersion), 2);

	/* A later edit with the current revision must survive a restart. */
	obs_data_set_int(seed_slot, core::keys::kClickLeftColor, 0x445566);
	obs_data_release(seed_slot);
	const std::string saved = obs_data_get_json(filter->settings);
	destroy_filter(ctx);

	obs_source_t *f2 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	obs_data_release(f2->settings);
	f2->settings = obs_data_create_from_json(saved.c_str());
	REQUIRE(f2->settings != nullptr);
	void *ctx2 = create_filter(f2);
	REQUIRE(ctx2 != nullptr);
	ts_filter_source_info.video_tick(ctx2, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(f2->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0x445566));
	obs_data_t *kept_slot = obs_data_get_obj(f2->settings, core::keys::kProfileSlots[0]);
	REQUIRE(kept_slot != nullptr);
	CHECK_EQ(obs_data_get_int(kept_slot, core::keys::kClickLeftColor),
		 static_cast<long long>(0x445566));
	obs_data_release(kept_slot);
	destroy_filter(ctx2);
}

/* Saved frontend bindings win; only missing profile hotkeys get the default
 * Ctrl+Shift+Alt+A..J. An explicitly empty binding stays empty. */
TEST(integration_hotkey_saved_bindings_and_defaults)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	fakeobs::set_profile_config_string(
		"Hotkeys", "TopScreen_ProfileA",
		R"({"bindings":[{"key":"OBS_KEY_B","control":true,"shift":true,"alt":true}]})");
	fakeobs::set_profile_config_string("Hotkeys", "TopScreen_ProfileC",
					   R"({"bindings":[]})");

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	const auto &saved_a = fakeobs::hotkey_bindings("TopScreen_ProfileA");
	REQUIRE(saved_a.size() == 1);
	CHECK_EQ(saved_a[0].key, OBS_KEY_B);
	CHECK_EQ(saved_a[0].modifiers,
		 static_cast<uint32_t>(INTERACT_CONTROL_KEY | INTERACT_SHIFT_KEY |
				       INTERACT_ALT_KEY));

	const auto &empty_c = fakeobs::hotkey_bindings("TopScreen_ProfileC");
	CHECK(empty_c.empty());

	const auto &default_j = fakeobs::hotkey_bindings("TopScreen_ProfileJ");
	REQUIRE(default_j.size() == 1);
	CHECK_EQ(default_j[0].key, OBS_KEY_J);
	CHECK_EQ(default_j[0].modifiers,
		 static_cast<uint32_t>(INTERACT_CONTROL_KEY | INTERACT_SHIFT_KEY |
				       INTERACT_ALT_KEY));

	/* Non-profile actions have no default. */
	CHECK(fakeobs::hotkey_bindings("TopScreen_ToggleEffects").empty());

	/* A profile switch reloads the saved bindings from the new OBS profile. */
	fakeobs::set_profile_config_string(
		"Hotkeys", "TopScreen_ProfileA",
		R"({"bindings":[{"key":"OBS_KEY_C","control":true,"shift":true,"alt":true}]})");
	fakeobs::fire_frontend_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
	const auto &switched_a = fakeobs::hotkey_bindings("TopScreen_ProfileA");
	REQUIRE(switched_a.size() == 1);
	CHECK_EQ(switched_a[0].key, OBS_KEY_C);

	/* The UI button forces all ten profile defaults and persists them. */
	Hotkeys::apply_default_profile_bindings();
	const auto &forced_a = fakeobs::hotkey_bindings("TopScreen_ProfileA");
	REQUIRE(forced_a.size() == 1);
	CHECK_EQ(forced_a[0].key, OBS_KEY_A);
	const auto &forced_c = fakeobs::hotkey_bindings("TopScreen_ProfileC");
	REQUIRE(forced_c.size() == 1);
	CHECK_EQ(forced_c[0].key, OBS_KEY_C);
	CHECK(fakeobs::profile_config_string("Hotkeys", "TopScreen_ProfileA")
		      .find("OBS_KEY_A") != std::string::npos);
	CHECK(fakeobs::profile_config_string("Hotkeys", "TopScreen_ProfileJ")
		      .find("OBS_KEY_J") != std::string::npos);

	destroy_filter(ctx);
}


/* The Restore default profiles button rewrites all slots and applies the
 * current one. */
TEST(integration_profile_reset_button_restores_seeds)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	ctrl->select_profile(3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	obs_data_t *slot = obs_data_get_obj(filter->settings, core::keys::kProfileSlots[3]);
	REQUIRE(slot != nullptr);
	obs_data_set_int(slot, core::keys::kClickLeftColor, 0x112233);
	obs_data_release(slot);

	REQUIRE(profiles_reset_from_properties(ctx));
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	obs_data_t *restored = obs_data_get_obj(filter->settings, core::keys::kProfileSlots[3]);
	REQUIRE(restored != nullptr);
	CHECK_EQ(obs_data_get_int(restored, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00D7FF));
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00D7FF));
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kProfileSeedVersion), 2);
	obs_data_release(restored);

	destroy_filter(ctx);
}
