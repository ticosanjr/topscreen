#include "core/config.hpp"
#include "core/preset.hpp"
#include "test_framework.hpp"

#include <cmath>
#include <limits>

using namespace topscreen::core;

TEST(config_sanitize_replaces_non_finite_values)
{
	Config cfg;
	const Config def;
	const double nan = std::numeric_limits<double>::quiet_NaN();
	const double inf = std::numeric_limits<double>::infinity();

	cfg.bg.gradient_angle = nan;
	cfg.bg.gradient_center_x = nan;
	cfg.bg.image_blur = nan;
	cfg.bg.padding_x = -inf;
	cfg.bg.shadow_alpha = inf;
	cfg.cursor.size = nan;
	cfg.zoom.amount = nan;
	cfg.spotlight.radius = nan;
	cfg.keys.ttl_ms = nan;
	cfg.highlight.opacity = nan;

	sanitize_config(&cfg);

	CHECK_NEAR(cfg.bg.gradient_angle, def.bg.gradient_angle, 1e-9);
	CHECK_NEAR(cfg.bg.gradient_center_x, def.bg.gradient_center_x, 1e-9);
	CHECK_NEAR(cfg.bg.image_blur, def.bg.image_blur, 1e-9);
	CHECK_NEAR(cfg.bg.padding_x, def.bg.padding_x, 1e-9); /* non-finite -> default */
	CHECK_NEAR(cfg.bg.shadow_alpha, def.bg.shadow_alpha, 1e-9);
	CHECK_NEAR(cfg.cursor.size, def.cursor.size, 1e-9);
	CHECK_NEAR(cfg.zoom.amount, def.zoom.amount, 1e-9);
	CHECK_NEAR(cfg.spotlight.radius, def.spotlight.radius, 1e-9);
	CHECK_NEAR(cfg.keys.ttl_ms, def.keys.ttl_ms, 1e-9);
	CHECK_NEAR(cfg.highlight.opacity, def.highlight.opacity, 1e-9);
}

TEST(config_sanitize_clamps_ranges_and_keeps_valid_values)
{
	Config cfg;
	cfg.bg.image_blur = 500.0;
	cfg.bg.image_blur_type = 99;
	cfg.bg.padding_x = -12.0;
	cfg.bg.shadow_offset_x = 500.0;
	cfg.cursor.size = 9.0;
	cfg.zoom.amount = 0.1;
	cfg.cursor.follow_mode = 99;
	cfg.cursor.glide_ms = 0.0;
	cfg.cursor.glide_sample_ms = 1.0;
	cfg.click_left.effect = 99;
	cfg.keys.max_keys = 50;
	cfg.highlight.thickness = 0.0;
	cfg.highlight.mode = 99;
	cfg.spotlight.radius = -1.0;

	sanitize_config(&cfg);

	CHECK_NEAR(cfg.bg.image_blur, 60.0, 1e-9);
	CHECK_EQ(cfg.bg.image_blur_type, 4);
	CHECK_NEAR(cfg.bg.padding_x, 0.0, 1e-9);
	CHECK_NEAR(cfg.bg.shadow_offset_x, 100.0, 1e-9);
	CHECK_NEAR(cfg.cursor.size, 4.0, 1e-9);
	CHECK_NEAR(cfg.zoom.amount, 1.0, 1e-9);
	CHECK_EQ(cfg.cursor.follow_mode, static_cast<int>(CursorFollowMode::GlideAB));
	CHECK_NEAR(cfg.cursor.glide_ms, 50.0, 1e-9);
	CHECK_NEAR(cfg.cursor.glide_sample_ms, 20.0, 1e-9);
	CHECK_EQ(cfg.click_left.effect, static_cast<int>(ClickEffect::WaterRipple));
	CHECK_EQ(cfg.keys.max_keys, 12);
	CHECK_NEAR(cfg.highlight.thickness, 0.001, 1e-9);
	CHECK_EQ(cfg.highlight.mode, static_cast<int>(HighlightMode::PressZoom));
	CHECK_NEAR(cfg.spotlight.radius, 0.05, 1e-9);

	/* A config already inside the UI ranges is left untouched. */
	Config valid;
	valid.bg.image_blur = 24.0;
	valid.bg.image_blur_type = 3;
	valid.bg.gradient_angle = 315.0;
	valid.cursor.size = 1.5;
	valid.zoom.amount = 2.0;
	sanitize_config(&valid);
	CHECK_NEAR(valid.bg.image_blur, 24.0, 1e-9);
	CHECK_EQ(valid.bg.image_blur_type, 3);
	CHECK_NEAR(valid.bg.gradient_angle, 315.0, 1e-9);
	CHECK_NEAR(valid.cursor.size, 1.5, 1e-9);
	CHECK_NEAR(valid.zoom.amount, 2.0, 1e-9);
}

TEST(config_sanitize_wraps_gradient_angle)
{
	Config cfg;
	cfg.bg.gradient_angle = -45.0;
	sanitize_config(&cfg);
	CHECK_NEAR(cfg.bg.gradient_angle, 315.0, 1e-9);

	cfg.bg.gradient_angle = 405.0;
	sanitize_config(&cfg);
	CHECK_NEAR(cfg.bg.gradient_angle, 45.0, 1e-9);

	/* Null is a no-op, not a crash. */
	sanitize_config(nullptr);
}

TEST(preset_apply_sanitizes_imported_values)
{
	const char *text = R"({
		"name": "Out of range",
		"settings": {
			"bg_image_blur": 999,
			"bg_image_blur_type": 42,
			"cursor_size": -3,
			"spotlight_radius": 4
		}
	})";

	Preset preset;
	std::string err;
	REQUIRE(Preset::from_json(text, &preset, &err));

	Config cfg;
	REQUIRE(preset.apply(&cfg, &err));

	CHECK_NEAR(cfg.bg.image_blur, 60.0, 1e-9);
	CHECK_EQ(cfg.bg.image_blur_type, 4);
	CHECK_NEAR(cfg.cursor.size, 0.5, 1e-9);
	CHECK_NEAR(cfg.spotlight.radius, 1.0, 1e-9);
}

TEST(config_sanitize_magnifier_and_spotlight_are_mutually_exclusive)
{
	Config cfg;
	cfg.spotlight.enabled = true;
	cfg.magnifier.enabled = true;
	sanitize_config(&cfg);
	CHECK(!cfg.spotlight.enabled);
	CHECK(cfg.magnifier.enabled);

	cfg.spotlight.enabled = true;
	cfg.magnifier.enabled = false;
	sanitize_config(&cfg);
	CHECK(cfg.spotlight.enabled);
	CHECK(!cfg.magnifier.enabled);

	Config raw;
	raw.magnifier.radius = 9.0;
	raw.magnifier.zoom_amount = 0.1;
	raw.magnifier.center_x = -4.0;
	sanitize_config(&raw);
	CHECK_NEAR(raw.magnifier.radius, 1.0, 1e-9);
	CHECK_NEAR(raw.magnifier.zoom_amount, 1.0, 1e-9);
	CHECK_NEAR(raw.magnifier.center_x, 0.0, 1e-9);
}
