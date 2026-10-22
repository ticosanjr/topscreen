#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

#include <string>
#include <utility>

#include "audio/ts-click-sound.hpp"
#include "core/keys.hpp"
#include "core/types.hpp"
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
	a.bg.mode = static_cast<int>(core::BgMode::Gradient);
	a.bg.gradient_angle = 135.0;
	a.bg.gradient_center_x = 0.3;
	a.bg.gradient_center_y = 0.7;
	a.bg.image_file = "wallpapers/fiber.jpg";
	a.bg.shadow_offset_x = -17.0;
	a.keys.pos_x = 0.25;
	a.highlight.color = 0x6600E0FFu;
	a.cursor.size = 1.5;
	a.cursor.secondary_style = static_cast<int>(core::CursorStyle::BibataHover);
	a.cursor.link_detection = false;
	a.cursor.follow_mode = static_cast<int>(core::CursorFollowMode::GlideSampled);
	a.cursor.glide_ms = 210.0;
	a.cursor.glide_sample_ms = 70.0;
	a.spotlight.shape = static_cast<int>(core::SpotlightShape::Square);
	a.spotlight.border_enabled = true;
	a.spotlight.border_color = 0xFF112233u;
	a.spotlight.shadow_enabled = true;
	a.spotlight.shadow_blur = 42.0;
	a.spotlight.blur_enabled = true;
	a.spotlight.blur_amount = 21.0;
	a.spotlight.zoom_amount = 1.7;
	a.spotlight.in_ms = 120.0;
	a.spotlight.drop_px = 18.0;
	a.magnifier.radius = 0.33;
	a.magnifier.zoom_amount = 2.2;
	a.magnifier.follow_cursor = false;
	a.magnifier.center_x = 0.25;
	a.magnifier.center_y = 0.75;
	a.magnifier.in_ms = 111.0;

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
	CHECK_NEAR(b.bg.gradient_angle, 135.0, 1e-9);
	CHECK_NEAR(b.bg.gradient_center_x, 0.3, 1e-9);
	CHECK_NEAR(b.bg.gradient_center_y, 0.7, 1e-9);
	CHECK_EQ(b.bg.image_file, a.bg.image_file);
	CHECK_NEAR(b.bg.shadow_offset_x, -17.0, 1e-9);
	CHECK_NEAR(b.keys.pos_x, 0.25, 1e-9);
	CHECK_EQ(b.highlight.color, a.highlight.color);
	CHECK_NEAR(b.cursor.size, 1.5, 1e-9);
	CHECK_EQ(b.cursor.secondary_style, a.cursor.secondary_style);
	CHECK(!b.cursor.link_detection);
	CHECK_EQ(b.cursor.follow_mode, a.cursor.follow_mode);
	CHECK_NEAR(b.cursor.glide_ms, 210.0, 1e-9);
	CHECK_NEAR(b.cursor.glide_sample_ms, 70.0, 1e-9);
	CHECK_EQ(b.spotlight.shape, a.spotlight.shape);
	CHECK(b.spotlight.border_enabled);
	CHECK_EQ(b.spotlight.border_color, a.spotlight.border_color);
	CHECK(b.spotlight.shadow_enabled);
	CHECK_NEAR(b.spotlight.shadow_blur, 42.0, 1e-9);
	CHECK(b.spotlight.blur_enabled);
	CHECK_NEAR(b.spotlight.blur_amount, 21.0, 1e-9);
	CHECK_NEAR(b.spotlight.zoom_amount, 1.7, 1e-9);
	CHECK_NEAR(b.spotlight.in_ms, 120.0, 1e-9);
	CHECK_NEAR(b.spotlight.drop_px, 18.0, 1e-9);
	CHECK_NEAR(b.magnifier.radius, 0.33, 1e-9);
	CHECK_NEAR(b.magnifier.zoom_amount, 2.2, 1e-9);
	CHECK(!b.magnifier.follow_cursor);
	CHECK_NEAR(b.magnifier.center_x, 0.25, 1e-9);
	CHECK_NEAR(b.magnifier.center_y, 0.75, 1e-9);
	CHECK_NEAR(b.magnifier.in_ms, 111.0, 1e-9);

	obs_data_release(s);
}

/* Optional fallback: fields absent from a partial profile slot keep the seed
 * instead of becoming zero/global defaults (used by profile import/export). */
TEST(settings_load_with_fallback_keeps_seed_fields)
{
	core::Config seed;
	seed.zoom.mode = static_cast<int>(core::ZoomMode::Off);
	seed.click_left.color = 0xFF00D6FFu;
	seed.bg.padding_x = 42.0;

	obs_data_t *s = obs_data_create();
	obs_data_set_int(s, core::keys::kCursorStyle, 60);

	core::Config out = seed;
	config_load(s, &out, &seed);

	CHECK_EQ(out.cursor.style, 60);
	CHECK_EQ(out.zoom.mode, seed.zoom.mode);
	CHECK_EQ(out.click_left.color, seed.click_left.color);
	CHECK_NEAR(out.bg.padding_x, 42.0, 1e-9);

	obs_data_release(s);
}

/* Legacy background modes are migrated without changing the visible look:
 * vertical/diagonal -> gradient (with the corresponding angle), image -> wallpaper.
 * A new explicit bg_type always wins, even if an old image path remains. */
TEST(settings_migrates_legacy_background_modes)
{
	{
		obs_data_t *s = obs_data_create();
		obs_data_set_int(s, core::keys::kBgMode, 1);
		core::Config c;
		config_load(s, &c);
		CHECK_EQ(c.bg.mode, static_cast<int>(core::BgMode::Gradient));
		CHECK_NEAR(c.bg.gradient_angle, 90.0, 1e-9);
		obs_data_release(s);
	}
	{
		obs_data_t *s = obs_data_create();
		obs_data_set_int(s, core::keys::kBgMode, 2);
		core::Config c;
		config_load(s, &c);
		CHECK_EQ(c.bg.mode, static_cast<int>(core::BgMode::Gradient));
		CHECK_NEAR(c.bg.gradient_angle, 45.0, 1e-9);
		obs_data_release(s);
	}
	{
		obs_data_t *s = obs_data_create();
		obs_data_set_int(s, core::keys::kBgMode, 0);
		obs_data_set_string(s, core::keys::kBgImage, "wallpapers/fiber.jpg");
		core::Config c;
		config_load(s, &c);
		CHECK_EQ(c.bg.mode, static_cast<int>(core::BgMode::Wallpaper));
		obs_data_release(s);
	}
	{
		obs_data_t *s = obs_data_create();
		obs_data_set_int(s, core::keys::kBgType, static_cast<int>(core::BgMode::Solid));
		obs_data_set_string(s, core::keys::kBgImage, "wallpapers/fiber.jpg");
		core::Config c;
		config_load(s, &c);
		CHECK_EQ(c.bg.mode, static_cast<int>(core::BgMode::Solid));
		obs_data_release(s);
	}
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

/* A deliberate Smoothing=0.7 + Sway=0.3 choice is valid and must survive a
 * reload (the old migration used to reset it to 0). */
TEST(settings_cursor_smooth_sway_are_preserved)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);
	obs_data_set_double(s, core::keys::kCursorSmooth, 0.7);
	obs_data_set_double(s, core::keys::kCursorSway, 0.3);

	core::Config c;
	config_load(s, &c);
	CHECK_NEAR(c.cursor.smooth, 0.7, 1e-9);
	CHECK_NEAR(c.cursor.sway, 0.3, 1e-9);

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
	CHECK(obs_properties_get(p, core::keys::kBgShadowOffsetX) != nullptr);
	CHECK(obs_properties_get(p, "grp_cursor") != nullptr);
	CHECK(obs_properties_get(p, "grp_zoom") != nullptr);
	CHECK(obs_properties_get(p, "grp_keys") != nullptr);
	CHECK(obs_properties_get(p, "grp_highlight") != nullptr);
	CHECK(obs_properties_get(p, "grp_spotlight") != nullptr);
	CHECK(obs_properties_get(p, core::keys::kSpotlightShape) != nullptr);
	CHECK(obs_properties_get(p, core::keys::kSpotlightBorderColor) != nullptr);
	CHECK(obs_properties_get(p, core::keys::kSpotlightShadowBlur) != nullptr);
	CHECK(obs_properties_get(p, core::keys::kSpotlightBlurAmount) != nullptr);
	CHECK(obs_properties_get(p, core::keys::kSpotlightZoomAmount) != nullptr);
	obs_properties_destroy(p);
}

/* The highlighter shows only the controls that apply to the current mode:
 * box/line get Look + Timing, zoom mode gets only the Zoom group. */
TEST(properties_highlight_groups_follow_the_mode)
{
	auto make_props = [](int mode, bool hotkey = true) {
		obs_data_t *s = obs_data_create();
		config_defaults(s);
		obs_data_set_int(s, core::keys::kHighlightMode, static_cast<long long>(mode));
		obs_data_set_bool(s, core::keys::kHighlightHotkeyEnabled, hotkey);
		return std::make_pair(s, properties_create(nullptr, s));
	};

	/* Default is box: look + timing visible. The independent hotkey also uses
	 * the zoom settings, so its group stays visible by default. */
	{
		auto [s, p] = make_props(static_cast<int>(core::HighlightMode::Box));
		CHECK(p != nullptr);
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_look")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_timing")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_zoom")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}

	/* With the hotkey disabled, a non-zoom drag mode hides the zoom group. */
	{
		auto [s, p] = make_props(static_cast<int>(core::HighlightMode::Box), false);
		CHECK(p != nullptr);
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_look")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_timing")));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, "grp_hl_zoom")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}

	/* Zoom mode: only the zoom group. */
	{
		auto [s, p] = make_props(static_cast<int>(core::HighlightMode::Zoom));
		CHECK(p != nullptr);
		CHECK(!fakeobs::property_visible(obs_properties_get(p, "grp_hl_look")));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, "grp_hl_timing")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_zoom")));
		CHECK(fakeobs::property_visible(
			obs_properties_get(p, core::keys::kHighlightZoomDelayMs)));
		CHECK(!fakeobs::property_visible(
			obs_properties_get(p, core::keys::kHighlightPressDelayMs)));
		obs_properties_destroy(p);
		obs_data_release(s);
	}

	/* PressZoom: no look/timing, zoom group with the press delay only. */
	{
		auto [s, p] = make_props(static_cast<int>(core::HighlightMode::PressZoom));
		CHECK(p != nullptr);
		CHECK(!fakeobs::property_visible(obs_properties_get(p, "grp_hl_look")));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, "grp_hl_timing")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_zoom")));
		CHECK(!fakeobs::property_visible(
			obs_properties_get(p, core::keys::kHighlightZoomDelayMs)));
		CHECK(fakeobs::property_visible(
			obs_properties_get(p, core::keys::kHighlightPressDelayMs)));
		obs_properties_destroy(p);
		obs_data_release(s);
	}

	/* Box+zoom shows everything: it draws the box AND zooms. */
	{
		auto [s, p] = make_props(static_cast<int>(core::HighlightMode::BoxZoom));
		CHECK(p != nullptr);
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_look")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_timing")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_zoom")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}

	/* Line mode behaves like box: look + timing, zoom group only via hotkey. */
	{
		auto [s, p] = make_props(static_cast<int>(core::HighlightMode::Line));
		CHECK(p != nullptr);
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_look")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_timing")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_zoom")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}

	{
		auto [s, p] = make_props(static_cast<int>(core::HighlightMode::Line), false);
		CHECK(p != nullptr);
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_look")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_timing")));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, "grp_hl_zoom")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}

	/* Line+zoom shows everything: it draws the stroke AND zooms. */
	{
		auto [s, p] = make_props(static_cast<int>(core::HighlightMode::LineZoom));
		CHECK(p != nullptr);
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_look")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_timing")));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_hl_zoom")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}
}

/* Background type controls which controls are shown: one color, two colors +
 * meeting point/angle, or the existing wallpaper controls. */
TEST(properties_background_type_visibility)
{
	auto make_props = [](int type) {
		obs_data_t *s = obs_data_create();
		config_defaults(s);
		obs_data_set_int(s, core::keys::kBgType, type);
		return std::make_pair(s, properties_create(nullptr, s));
	};

	{
		auto [s, p] = make_props(static_cast<int>(core::BgMode::Solid));
		CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kBgColorA)));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kBgColorB)));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kBgGradientAngle)));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, "grp_bg_image")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}
	{
		auto [s, p] = make_props(static_cast<int>(core::BgMode::Gradient));
		CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kBgColorA)));
		CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kBgColorB)));
		CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kBgGradientAngle)));
		CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kBgGradientCenterX)));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, "grp_bg_image")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}
	{
		auto [s, p] = make_props(static_cast<int>(core::BgMode::Wallpaper));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kBgColorA)));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kBgColorB)));
		CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kBgGradientAngle)));
		CHECK(fakeobs::property_visible(obs_properties_get(p, "grp_bg_image")));
		obs_properties_destroy(p);
		obs_data_release(s);
	}
}

/* Dependent controls hide when their master switch makes them irrelevant. */
TEST(properties_dependent_controls_visibility)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);
	obs_data_set_bool(s, core::keys::kCursorHideIdle, true);
	obs_data_set_bool(s, core::keys::kCursorLoop, false);
	obs_data_set_int(s, core::keys::kSpotlightShape,
			 static_cast<long long>(core::SpotlightShape::Circle));
	obs_data_set_bool(s, core::keys::kKeysBand, true);

	obs_properties_t *p = properties_create(nullptr, s);
	CHECK(p != nullptr);
	CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kCursorIdleDelayMs)));
	CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kCursorLoopMs)));
	CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kSpotlightCornerRadius)));
	CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kKeysBandColor)));

	/* Toggle the masters and run the same callbacks OBS would run. */
	obs_data_set_bool(s, core::keys::kCursorHideIdle, false);
	obs_data_set_bool(s, core::keys::kCursorLoop, true);
	fakeobs::invoke_property_modified(p, obs_properties_get(p, core::keys::kCursorHideIdle), s);
	fakeobs::invoke_property_modified(p, obs_properties_get(p, core::keys::kCursorLoop), s);
	CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kCursorIdleDelayMs)));
	CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kCursorLoopMs)));

	obs_data_set_int(s, core::keys::kSpotlightShape,
			 static_cast<long long>(core::SpotlightShape::Square));
	fakeobs::invoke_property_modified(p, obs_properties_get(p, core::keys::kSpotlightShape), s);
	CHECK(fakeobs::property_visible(obs_properties_get(p, core::keys::kSpotlightCornerRadius)));

	obs_data_set_bool(s, core::keys::kKeysBand, false);
	fakeobs::invoke_property_modified(p, obs_properties_get(p, core::keys::kKeysBand), s);
	CHECK(!fakeobs::property_visible(obs_properties_get(p, core::keys::kKeysBandColor)));

	obs_properties_destroy(p);
	obs_data_release(s);
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
	CHECK(!snd.load("sounds/does-not-exist.wav", "sounds/does-not-exist.wav"));
	snd.play(false, 0.6);
	snd.play(true, 0.0);
	CHECK(!snd.loaded());

	CHECK(snd.load("sounds/click-soft.wav", "sounds/click-mouse-right.wav"));
	CHECK(snd.loaded());
	CHECK(snd.wav().sample_rate > 0);
	CHECK(snd.loaded_path(false).find("click-soft.wav") != std::string::npos);
	CHECK(snd.loaded_path(true).find("click-mouse-right.wav") != std::string::npos);

	/* Changing only the left file reloads just that side. */
	CHECK(snd.load("sounds/click-bubble.wav", "sounds/click-mouse-right.wav"));
	CHECK(snd.loaded_path(false).find("click-bubble.wav") != std::string::npos);
	CHECK(snd.loaded_path(true).find("click-mouse-right.wav") != std::string::npos);
	/* Does not actually play in the test (avoids fork/exec in the suite). */
	snd.release();
	CHECK(!snd.loaded());
}

/* Persisted styles outside the atlas range must never reach the renderer. */
TEST(settings_cursor_style_is_clamped_and_retired_resolved)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);
	core::Config c;

	obs_data_set_int(s, core::keys::kCursorStyle, -5);
	config_load(s, &c);
	CHECK_EQ(c.cursor.style, 0);

	obs_data_set_int(s, core::keys::kCursorStyle, 999);
	config_load(s, &c);
	CHECK_EQ(c.cursor.style, static_cast<int>(core::CursorStyle::CustomPurpleIris));

	obs_data_set_int(s, core::keys::kCursorStyle,
			 static_cast<int>(core::CursorStyle::BreezeIBeam));
	config_load(s, &c);
	CHECK_EQ(c.cursor.style, static_cast<int>(core::CursorStyle::Breeze));
	obs_data_release(s);
}

/* The top of the Properties shows the version, and the Profiles group carries
 * the backup/transfer controls (dock fallback for builds without Qt). */
TEST(properties_version_and_profile_backup_are_present)
{
	obs_properties_t *p = properties_create(nullptr);
	REQUIRE(p != nullptr);
	CHECK(obs_properties_get(p, "topscreen_version") != nullptr);
	CHECK(obs_properties_get(p, "grp_profile_backup") != nullptr);
	CHECK(obs_properties_get(p, "profile_export_path") != nullptr);
	CHECK(obs_properties_get(p, "profile_import_path") != nullptr);
	CHECK(obs_properties_get(p, "profile_export_btn") != nullptr);
	CHECK(obs_properties_get(p, "profile_import_btn") != nullptr);
	obs_properties_destroy(p);
}
