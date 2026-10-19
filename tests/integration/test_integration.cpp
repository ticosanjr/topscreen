#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

#include <cstdlib>
#include <string>

#if defined(_WIN32)
#define TS_SETENV(k, v) _putenv_s(k, v)
#define TS_UNSETENV(k) _putenv_s(k, "")
#else
#define TS_SETENV(k, v) ::setenv(k, v, 1)
#define TS_UNSETENV(k) ::unsetenv(k)
#endif

#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "core/types.hpp"
#include "ts-filter.hpp"
#include "ts-registry.hpp"

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
	return ts_filter_source_info.create(filter->settings, filter);
}

/* Simulates the profile list in Properties: writes profile_current and calls update. */
void set_profile_from_properties(obs_source_t *filter, void *ctx, int index)
{
	obs_data_set_int(filter->settings, core::keys::kProfileCurrent, index);
	ts_filter_source_info.update(ctx, filter->settings);
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
	const int toggle = fakeobs::find_hotkey("TopScreen.ToggleEffects");
	REQUIRE(toggle >= 0);
	fakeobs::invoke_hotkey(toggle, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->effects_enabled());

	/* Zoom via hotkey must not crash anything. */
	const int zin = fakeobs::find_hotkey("TopScreen.ZoomIn");
	REQUIRE(zin >= 0);
	fakeobs::invoke_hotkey(zin, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Preset via FilterControl (UI thread -> applied on tick). */
	ctrl->apply_preset(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_preset(), 1);
	/* tutorial.json uses a solid background (bg_mode = 0). */
	CHECK_EQ(static_cast<int>(obs_data_get_int(filter->settings, core::keys::kBgMode)), 0);

	ts_filter_source_info.destroy(ctx);
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

	const int tg = fakeobs::find_hotkey("TopScreen.ToggleBackground");
	REQUIRE(tg >= 0);
	const bool before = obs_data_get_bool(filter->settings, core::keys::kBgEnabled);
	fakeobs::invoke_hotkey(tg, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!obs_data_get_bool(filter->settings, core::keys::kBgEnabled) != !before);

	ts_filter_source_info.destroy(ctx);
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

	const char *names[] = {"TopScreen.ToggleEffects", "TopScreen.ToggleBackground",
			       "TopScreen.ZoomIn",	      "TopScreen.ZoomOut",
			       "TopScreen.ResetZoom",	      "TopScreen.NextPreset",
			       "TopScreen.PrevPreset",	      "TopScreen.ProfileA",
			       "TopScreen.ProfileB",	      "TopScreen.ProfileC",
			       "TopScreen.ProfileD",	      "TopScreen.ProfileE",
			       "TopScreen.ProfileF",	      "TopScreen.ProfileG",
			       "TopScreen.ProfileH",	      "TopScreen.ProfileI",
			       "TopScreen.ProfileJ",	      "TopScreen.ProfileSave"};
	for (const char *n : names)
		REQUIRE(fakeobs::find_hotkey(n) >= 0);

	/* pressed=false does nothing (callback early-return). */
	const bool fx0 = ctrl->effects_enabled();
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ToggleEffects"), false);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ProfileC"), false);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->effects_enabled() == fx0);
	CHECK_EQ(ctrl->current_profile(), 0);

	/* pressed=true: each action reaches the filter. */
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ToggleEffects"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->effects_enabled() == !fx0);

	const bool bg0 = obs_data_get_bool(filter->settings, core::keys::kBgEnabled);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ToggleBackground"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_bool(filter->settings, core::keys::kBgEnabled) == !bg0);

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ZoomIn"), true);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ZoomOut"), true);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ResetZoom"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.NextPreset"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->current_preset() >= 0 && ctrl->current_preset() < ctrl->preset_count());
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.PrevPreset"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->current_preset() >= 0 && ctrl->current_preset() < ctrl->preset_count());

	const char *profiles[] = {"TopScreen.ProfileA", "TopScreen.ProfileB", "TopScreen.ProfileC",
				   "TopScreen.ProfileD", "TopScreen.ProfileE", "TopScreen.ProfileF",
				   "TopScreen.ProfileG", "TopScreen.ProfileH", "TopScreen.ProfileI",
				   "TopScreen.ProfileJ"};
	for (int i = 0; i < 10; ++i) {
		fakeobs::invoke_hotkey(fakeobs::find_hotkey(profiles[i]), true);
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		CHECK_EQ(ctrl->current_profile(), i);
	}

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ProfileSave"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_obj(filter->settings, core::keys::kProfileSlots[ctrl->current_profile()]) !=
	      nullptr);

	ts_filter_source_info.destroy(ctx);
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

	ts_filter_source_info.destroy(ctx);
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

	ts_filter_source_info.destroy(ctx);
	CHECK(registry_first() == nullptr);
}

/* Creating the filter must register every hotkey on the source, once each,
 * with the stable TopScreen.* ids (the OBS UI groups them under the filter
 * instance name, so a missing section means these never registered). */
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

	ts_filter_source_info.destroy(ctx);

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

	ts_filter_source_info.destroy(ctx2);
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

	ts_filter_source_info.destroy(ctx);
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

	const char *names[] = {"TopScreen.ToggleEffects", "TopScreen.ToggleBackground",
			       "TopScreen.ZoomIn",	      "TopScreen.ZoomOut",
			       "TopScreen.ResetZoom",	      "TopScreen.NextPreset",
			       "TopScreen.PrevPreset",	      "TopScreen.ProfileA",
			       "TopScreen.ProfileB",	      "TopScreen.ProfileC",
			       "TopScreen.ProfileD",	      "TopScreen.ProfileE",
			       "TopScreen.ProfileF",	      "TopScreen.ProfileG",
			       "TopScreen.ProfileH",	      "TopScreen.ProfileI",
			       "TopScreen.ProfileJ",	      "TopScreen.ProfileSave"};
	const int expected = static_cast<int>(sizeof(names) / sizeof(names[0]));

	/* The harness keeps earlier registrations, so measure this filter's delta. */
	CHECK_EQ(fakeobs::hotkey_count() - before, expected);
	for (const char *n : names)
		CHECK(fakeobs::find_hotkey(n) >= 0);

	/* Every callback must be wired and active (a null func would make the UI
	 * list an entry that never fires). */
	CHECK(fakeobs::hotkeys_active_from(before));

	ts_filter_source_info.destroy(ctx);
	CHECK(!fakeobs::hotkeys_active_from(before));
}

/* Frontend hotkeys are process-wide: a second filter must NOT register a
 * second set of 18, and destroying one filter must keep them alive. */
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

	CHECK_EQ(fakeobs::hotkey_count() - before, 18);
	CHECK(fakeobs::hotkeys_active_from(before));

	/* Destroying one filter keeps the shared hotkeys working. */
	ts_filter_source_info.destroy(ctx1);
	CHECK(fakeobs::hotkeys_active_from(before));

	ts_filter_source_info.destroy(ctx2);
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

	/* Profile A (default): zoom 1.6 and white left click. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.6, 1e-6);

	/* Profile B: zoom 2.0 and GREEN left click (seeded). */
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 2.0, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00FF00));

	/* Going back to A must restore EVERYTHING. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.6, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFFFFFF));

	ts_filter_source_info.destroy(ctx);
}

TEST(integration_profile_from_list_restores_everything)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Profile B (seeded): zoom 2.0 + green left click. */
	set_profile_from_properties(filter, ctx, 1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 2.0, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00FF00));

	/* The main keys must come along, otherwise the next reload undoes the profile. */
	CHECK(obs_data_has_user_value(filter->settings, core::keys::kZoomAmount));

	/* Changing ANOTHER property must not undo the active profile. */
	obs_data_set_bool(filter->settings, core::keys::kHighlightEnabled, false);
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 2.0, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00FF00));

	/* Profile C (seeded): purple/cyan + zoom 1.4. */
	set_profile_from_properties(filter, ctx, 2);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.4, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFF5C7C));

	ts_filter_source_info.destroy(ctx);
	CHECK(registry_first() == nullptr);
}

TEST(integration_profile_keeps_user_edit)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Goes to profile D (zoom off). */
	set_profile_from_properties(filter, ctx, 3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kZoomMode),
		 static_cast<long long>(core::ZoomMode::Off));

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

	ts_filter_source_info.destroy(ctx);
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
	ts_filter_source_info.destroy(ctx);

	/* "Reopens OBS": the saved collection has profile_current + the slots. */
	obs_data_t saved = *filter->settings;
	obs_source_t *f2 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	*f2->settings = saved;
	void *ctx2 = ts_filter_source_info.create(f2->settings, f2);
	REQUIRE(ctx2 != nullptr);
	ts_filter_source_info.video_tick(ctx2, 1.0f / 60.0f);

	/* The state must be that of profile B (not the default). */
	CHECK_NEAR(obs_data_get_double(f2->settings, core::keys::kZoomAmount), 2.0, 1e-6);
	CHECK_EQ(obs_data_get_int(f2->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00FF00));

	ts_filter_source_info.destroy(ctx2);
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

	ts_filter_source_info.destroy(ctx);
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
	ts_filter_source_info.destroy(ctx);
}
