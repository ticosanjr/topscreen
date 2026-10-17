#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

#include <string>

#include "audio/ts-click-sound.hpp"
#include "core/keys.hpp"
#include "input/ts-input.hpp"
#include "ts-properties.hpp"
#include "ts-settings.hpp"

using namespace topscreen;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

/* Code defaults == default Config, and defaults do not become user values. */
TEST(settings_defaults_match_config)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);

	CHECK(!obs_data_has_user_value(s, core::keys::kZoomAmount));
	CHECK(!obs_data_has_user_value(s, core::keys::kProfileCurrent));
	CHECK(obs_data_has_default_value(s, core::keys::kZoomAmount));

	core::Config c;
	config_load(s, &c);

	const core::Config d;
	CHECK_NEAR(c.zoom.amount, d.zoom.amount, 1e-9);
	CHECK_EQ(c.zoom.mode, d.zoom.mode);
	CHECK_EQ(c.click_left.color, d.click_left.color);
	CHECK_EQ(c.click_right.color, d.click_right.color);
	CHECK_EQ(c.bg.mode, d.bg.mode);
	CHECK(c.effects_enabled == d.effects_enabled);
	CHECK(c.cursor.enabled == d.cursor.enabled);

	obs_data_release(s);
}

/* Old source (no keys at all): falls back to the code defaults. */
TEST(settings_old_source_uses_defaults)
{
	obs_data_t *s = obs_data_create();

	core::Config c;
	c.zoom.amount = -1.0;
	config_load(s, &c);

	const core::Config d;
	CHECK_NEAR(c.zoom.amount, d.zoom.amount, 1e-9);
	CHECK_EQ(c.click_left.color, d.click_left.color);

	obs_data_release(s);
}

/* Store -> load preserves the fields (roundtrip). */
TEST(settings_store_load_roundtrip)
{
	core::Config a;
	a.effects_enabled = false;
	a.zoom.mode = static_cast<int>(core::ZoomMode::Off);
	a.zoom.amount = 2.5;
	a.click_left.color = 0xFF112233u;
	a.click_right.color = 0xFF445566u;
	a.bg.mode = static_cast<int>(core::BgMode::Vertical);
	a.bg.image_file = "wallpapers/ocean.jpg";
	a.keys.pos_x = 0.25;
	a.highlight.color = 0x6600E0FFu;
	a.cursor.size = 1.5;

	obs_data_t *s = obs_data_create();
	config_store(s, a);

	core::Config b;
	config_load(s, &b);

	CHECK(b.effects_enabled == false);
	CHECK_EQ(b.zoom.mode, a.zoom.mode);
	CHECK_NEAR(b.zoom.amount, 2.5, 1e-9);
	CHECK_EQ(b.click_left.color, a.click_left.color);
	CHECK_EQ(b.click_right.color, a.click_right.color);
	CHECK_EQ(b.bg.mode, a.bg.mode);
	CHECK_EQ(b.bg.image_file, a.bg.image_file);
	CHECK_NEAR(b.keys.pos_x, 0.25, 1e-9);
	CHECK_EQ(b.highlight.color, a.highlight.color);
	CHECK_NEAR(b.cursor.size, 1.5, 1e-9);

	obs_data_release(s);
}

/* Migration of the old bg_padding key. */
TEST(settings_migrates_old_bg_padding)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);
	obs_data_set_double(s, "bg_padding", 100.0);

	core::Config c;
	config_load(s, &c);
	CHECK_NEAR(c.bg.padding_x, 100.0, 1e-9);
	CHECK_NEAR(c.bg.padding_y, 100.0, 1e-9);

	obs_data_release(s);
}

/* Migration of the old cursor smoothing defaults. */
TEST(settings_migrates_old_cursor_smooth)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);
	obs_data_set_double(s, core::keys::kCursorSmooth, 0.7);
	obs_data_set_double(s, core::keys::kCursorSway, 0.3);

	core::Config c;
	config_load(s, &c);
	CHECK_NEAR(c.cursor.smooth, 0.0, 1e-9);
	CHECK_NEAR(c.cursor.sway, 0.0, 1e-9);

	obs_data_release(s);
}

/* Properties: profile group at the top + main keys present. */
TEST(properties_creates_expected_groups)
{
	obs_properties_t *p = properties_create(nullptr);
	CHECK(p != nullptr);
	CHECK(obs_properties_get(p, "grp_profiles") != nullptr);
	CHECK(obs_properties_get(p, core::keys::kEffectsEnabled) != nullptr);
	CHECK(obs_properties_get(p, "grp_bg") != nullptr);
	CHECK(obs_properties_get(p, "grp_cursor") != nullptr);
	CHECK(obs_properties_get(p, "grp_zoom") != nullptr);
	CHECK(obs_properties_get(p, "grp_keys") != nullptr);
	CHECK(obs_properties_get(p, "grp_highlight") != nullptr);
	obs_properties_destroy(p);
}

/* Input without backend: available=false, tier 0, no phantom edges. */
TEST(input_without_backend_is_safe)
{
	Input in;
	in.poll();
	CHECK(!in.available());
	CHECK_EQ(in.tier(), 0);
	in.poll();
	CHECK(!in.available());
}

/* Sound: nonexistent file fails without crashing; play without load is safe. */
TEST(clicksound_fails_gracefully)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	ClickSound snd;
	CHECK(!snd.loaded());
	CHECK(!snd.load("sounds/does-not-exist.wav"));
	snd.play();
	CHECK(!snd.loaded());

	CHECK(snd.load("sounds/click-soft.wav"));
	CHECK(snd.loaded());
	CHECK(snd.wav().sample_rate > 0);
	/* Does not actually play in the test (avoids fork/exec in the suite). */
	snd.release();
	CHECK(!snd.loaded());
}
