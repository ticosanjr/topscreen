#include "core/preset.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

/*
 * Ensures that preset.apply() covers ALL configuration keys.
 *
 * If someone adds a field to Config and forgets to map it here, this test
 * fails. The values below are all different from the defaults, so every field
 * "must have changed".
 */
TEST(preset_covers_all_keys)
{
	const std::string text = R"({
        "name": "All",
        "settings": {
            "effects_enabled": false,
            "bg_enabled": false,
            "bg_mode": 1,
            "bg_type": 1,
            "bg_gradient_angle": 135,
            "bg_gradient_center_x": 0.3,
            "bg_gradient_center_y": 0.7,
            "bg_color_a": 111,
            "bg_color_b": 222,
            "bg_image": "wallpapers/fiber.jpg",
            "bg_image_file": "/tmp/my-image.jpg",
            "bg_image_blur": 9,
            "bg_padding_x": 11,
            "bg_padding_y": 12,
            "bg_corner_radius": 12,
            "bg_shadow": false,
            "bg_shadow_alpha": 0.11,
            "bg_shadow_blur": 13,
            "bg_shadow_offset_x": -9,
            "bg_shadow_offset": 6,
            "bg_keep_aspect": false,
            "cursor_enabled": false,
            "cursor_style": 1,
            "cursor_secondary_style": 26,
            "cursor_link_detection": false,
            "cursor_size": 2.5,
            "cursor_rotation_offset_deg": -12.5,
            "cursor_shadow": false,
            "cursor_follow_mode": 2,
            "cursor_glide_ms": 240,
            "cursor_hide_idle": false,
            "cursor_idle_delay_ms": 333,
            "zoom_mode": 2,
            "zoom_amount": 3.3,
            "zoom_duration_ms": 222,
            "zoom_idle_timeout_ms": 555,
            "zoom_pan_amount": 0.77,
            "zoom_center_amount": 0.5,
            "click_left_effect": 8,
            "click_left_size": 1.7,
            "click_left_color": 999,
            "click_left_duration_ms": 123,
            "click_right_effect": 7,
            "click_right_size": 0.4,
            "click_right_color": 888,
            "click_right_duration_ms": 321,
            "click_sound_enabled": false,
            "click_sound_file": "x.wav",
            "click_sound_file_left": "l.wav",
            "click_sound_file_right": "r.wav",
            "click_sound_volume": 0.42,
            "click_sound_left": false,
            "click_sound_right": true,
            "spotlight_enabled": false,
            "spotlight_radius": 0.44,
            "spotlight_dim": 0.33,
            "spotlight_shape": 1,
            "spotlight_corner_radius": 16,
            "spotlight_border_enabled": true,
            "spotlight_border_color": 4278255360,
            "spotlight_border_width": 6,
            "spotlight_blur_enabled": true,
            "spotlight_blur_amount": 18,
            "spotlight_zoom_amount": 1.75,
            "spotlight_in_ms": 180,
            "spotlight_out_ms": 230,
            "spotlight_show_cursor": false,
            "magnifier_enabled": true,
            "magnifier_shape": 1,
            "magnifier_radius": 0.35,
            "magnifier_corner_radius": 12,
            "magnifier_zoom": 2.5,
            "magnifier_border_enabled": true,
            "magnifier_border_color": 4278255360,
            "magnifier_border_width": 7,
            "magnifier_shadow_enabled": true,
            "magnifier_shadow_color": 4278190080,
            "magnifier_shadow_alpha": 0.61,
            "magnifier_shadow_blur": 26,
            "magnifier_shadow_offset_x": 4,
            "magnifier_shadow_offset_y": 8,
            "magnifier_follow_cursor": false,
            "magnifier_center_x": 0.4,
            "magnifier_center_y": 0.6,
            "magnifier_in_ms": 190,
            "magnifier_out_ms": 240,
            "magnifier_show_cursor": false,
            "keys_enabled": false,
            "keys_modifiers_only": true,
            "keys_show_special": false,
            "keys_show_shortcuts": false,
            "keys_ttl_ms": 1234,
            "keys_max": 9,
            "keys_pos_x": 0.25,
            "keys_pos_y": 0.8,
            "keys_size": 1.5,
            "keys_font": "fonts/DejaVuSans.ttf",
            "keys_color": 255,
            "keys_shadow": false,
            "keys_band": false,
            "keys_band_color": 1234,
            "highlight_enabled": false,
            "highlight_hotkey_enabled": false,
            "highlight_hold_ms": 777,
            "highlight_fade_ms": 222,
            "highlight_color": 12345,
            "highlight_mode": 2,
            "highlight_thickness": 0.012,
            "highlight_zoom_delay_ms": 1500,
            "highlight_zoom_max": 3.5,
            "highlight_zoom_in_ms": 120,
            "highlight_zoom_hold_ms": 400,
            "highlight_zoom_out_ms": 2000,
            "monitor_manual": true,
            "monitor_x": -1920,
            "monitor_y": 1080
        }
    })";

	Preset p;
	std::string err;
	REQUIRE(Preset::from_json(text, &p, &err));

	Config c;
	REQUIRE(p.apply(&c, &err));

	CHECK(!c.effects_enabled);

	CHECK(!c.bg.enabled);
	CHECK_EQ(c.bg.mode, 1);
	CHECK_NEAR(c.bg.gradient_angle, 135.0, 1e-9);
	CHECK_NEAR(c.bg.gradient_center_x, 0.3, 1e-9);
	CHECK_NEAR(c.bg.gradient_center_y, 0.7, 1e-9);
	CHECK_EQ(c.bg.color_a, 111u);
	CHECK_EQ(c.bg.color_b, 222u);
	CHECK_EQ(c.bg.image, std::string("wallpapers/fiber.jpg"));
	CHECK_EQ(c.bg.image_file, std::string("/tmp/my-image.jpg"));
	CHECK_NEAR(c.bg.image_blur, 9.0, 1e-9);
	CHECK_NEAR(c.bg.padding_x, 11.0, 1e-9);
	CHECK_NEAR(c.bg.padding_y, 12.0, 1e-9);
	CHECK_NEAR(c.bg.corner_radius, 12.0, 1e-9);
	CHECK(!c.bg.shadow);
	CHECK_NEAR(c.bg.shadow_alpha, 0.11, 1e-9);
	CHECK_NEAR(c.bg.shadow_blur, 13.0, 1e-9);
	CHECK_NEAR(c.bg.shadow_offset_x, -9.0, 1e-9);
	CHECK_NEAR(c.bg.shadow_offset, 6.0, 1e-9);
	CHECK(!c.bg.keep_aspect);

	CHECK(!c.cursor.enabled);
	CHECK_EQ(c.cursor.style, 1);
	CHECK_EQ(c.cursor.secondary_style, 26);
	CHECK(!c.cursor.link_detection);
	CHECK_NEAR(c.cursor.size, 2.5, 1e-9);
	CHECK_NEAR(c.cursor.rotation_offset_deg, -12.5, 1e-9);
	CHECK(!c.cursor.shadow);
	CHECK_EQ(c.cursor.follow_mode, 3);
	CHECK_NEAR(c.cursor.glide_ms, 240.0, 1e-9);
	CHECK(!c.cursor.hide_idle);
	CHECK_NEAR(c.cursor.idle_delay_ms, 333.0, 1e-9);

	CHECK_EQ(c.zoom.mode, 0);
	CHECK_NEAR(c.zoom.amount, 3.3, 1e-9);
	CHECK_NEAR(c.zoom.duration_ms, 222.0, 1e-9);
	CHECK_NEAR(c.zoom.idle_timeout_ms, 555.0, 1e-9);
	CHECK_NEAR(c.zoom.pan_amount, 0.77, 1e-9);
	CHECK_NEAR(c.zoom.center_amount, 0.5, 1e-9);

	CHECK_EQ(c.click_left.effect, 8);
	CHECK_NEAR(c.click_left.size, 1.7, 1e-9);
	CHECK_EQ(c.click_left.color, 999u);
	CHECK_NEAR(c.click_left.duration_ms, 123.0, 1e-9);

	CHECK_EQ(c.click_right.effect, 7);
	CHECK_NEAR(c.click_right.size, 0.4, 1e-9);
	CHECK_EQ(c.click_right.color, 888u);
	CHECK_NEAR(c.click_right.duration_ms, 321.0, 1e-9);

	CHECK(!c.click_sound.enabled);
	CHECK_EQ(c.click_sound.file_left, std::string()); /* legacy left=false -> mute */
	CHECK_EQ(c.click_sound.file_right, std::string("r.wav"));
	CHECK_NEAR(c.click_sound.volume, 0.42, 1e-9);
	CHECK(!c.click_sound.left);
	CHECK(c.click_sound.right);


	CHECK(!c.spotlight.enabled); /* magnifier wins in the mutual-exclusion pass */
	CHECK_NEAR(c.spotlight.radius, 0.44, 1e-9);
	CHECK_NEAR(c.spotlight.dim, 0.33, 1e-9);
	CHECK_EQ(c.spotlight.shape, 1);
	CHECK_NEAR(c.spotlight.corner_radius, 16.0, 1e-9);
	CHECK(c.spotlight.border_enabled);
	CHECK_EQ(c.spotlight.border_color, 4278255360u);
	CHECK_NEAR(c.spotlight.border_width, 6.0, 1e-9);
	CHECK(c.spotlight.blur_enabled);
	CHECK_NEAR(c.spotlight.blur_amount, 18.0, 1e-9);
	CHECK_NEAR(c.spotlight.zoom_amount, 1.75, 1e-9);
	CHECK_NEAR(c.spotlight.in_ms, 180.0, 1e-9);
	CHECK_NEAR(c.spotlight.out_ms, 230.0, 1e-9);
	CHECK(!c.spotlight.show_cursor);

	CHECK(c.magnifier.enabled);
	CHECK_EQ(c.magnifier.shape, 1);
	CHECK_NEAR(c.magnifier.radius, 0.35, 1e-9);
	CHECK_NEAR(c.magnifier.corner_radius, 12.0, 1e-9);
	CHECK_NEAR(c.magnifier.zoom_amount, 2.5, 1e-9);
	CHECK(c.magnifier.border_enabled);
	CHECK_EQ(c.magnifier.border_color, 4278255360u);
	CHECK_NEAR(c.magnifier.border_width, 7.0, 1e-9);
	CHECK(c.magnifier.shadow_enabled);
	CHECK_EQ(c.magnifier.shadow_color, 4278190080u);
	CHECK_NEAR(c.magnifier.shadow_alpha, 0.61, 1e-9);
	CHECK_NEAR(c.magnifier.shadow_blur, 26.0, 1e-9);
	CHECK_NEAR(c.magnifier.shadow_offset_x, 4.0, 1e-9);
	CHECK_NEAR(c.magnifier.shadow_offset_y, 8.0, 1e-9);
	CHECK(!c.magnifier.follow_cursor);
	CHECK_NEAR(c.magnifier.center_x, 0.4, 1e-9);
	CHECK_NEAR(c.magnifier.center_y, 0.6, 1e-9);
	CHECK_NEAR(c.magnifier.in_ms, 190.0, 1e-9);
	CHECK_NEAR(c.magnifier.out_ms, 240.0, 1e-9);
	CHECK(!c.magnifier.show_cursor);

	CHECK(!c.keys.enabled);
	CHECK(c.keys.modifiers_only);
	CHECK(!c.keys.show_special);
	CHECK(!c.keys.show_shortcuts);
	CHECK_NEAR(c.keys.ttl_ms, 1234.0, 1e-9);
	CHECK_EQ(c.keys.max_keys, 9);
	CHECK_NEAR(c.keys.pos_x, 0.25, 1e-9);
	CHECK_NEAR(c.keys.pos_y, 0.8, 1e-9);
	CHECK_NEAR(c.keys.size, 1.5, 1e-9);
	CHECK_EQ(c.keys.font, std::string("fonts/DejaVuSans.ttf"));
	CHECK_EQ(c.keys.color, 255u);
	CHECK(!c.keys.shadow);
	CHECK(!c.keys.band);
	CHECK_EQ(c.keys.band_color, 1234u);

	CHECK(!c.highlight.enabled);
	CHECK(!c.highlight.hotkey_enabled);
	CHECK_NEAR(c.highlight.hold_ms, 777.0, 1e-9);
	CHECK_NEAR(c.highlight.fade_ms, 222.0, 1e-9);
	CHECK_EQ(c.highlight.color, 12345u);

	CHECK_EQ(c.highlight.mode, 2);
	CHECK_NEAR(c.highlight.thickness, 0.012, 1e-9);
	CHECK_NEAR(c.highlight.zoom_delay_ms, 1500.0, 1e-9);
	CHECK_NEAR(c.highlight.zoom_max, 3.5, 1e-9);
	CHECK_NEAR(c.highlight.zoom_in_ms, 120.0, 1e-9);
	CHECK_NEAR(c.highlight.zoom_hold_ms, 400.0, 1e-9);
	CHECK_NEAR(c.highlight.zoom_out_ms, 2000.0, 1e-9);

	CHECK(c.monitor_manual);
	CHECK_EQ(c.monitor_x, -1920);
	CHECK_EQ(c.monitor_y, 1080);
}

/* Local image and Transparent are background types, not wallpaper list
 * entries. A preset saved with the new numeric types must apply directly. */
TEST(preset_accepts_local_and_transparent_background_types)
{
	{
		const std::string text = R"({
            "name": "Local",
            "settings": { "bg_type": 3, "bg_image_file": "/tmp/p.png" }
        })";
		Preset p;
		std::string err;
		REQUIRE(Preset::from_json(text, &p, &err));
		Config c;
		REQUIRE(p.apply(&c, &err));
		CHECK_EQ(c.bg.mode, static_cast<int>(BgMode::LocalImage));
		CHECK_EQ(c.bg.image_file, std::string("/tmp/p.png"));
	}
	{
		const std::string text = R"({
            "name": "Transparent",
            "settings": { "bg_type": 4 }
        })";
		Preset p;
		std::string err;
		REQUIRE(Preset::from_json(text, &p, &err));
		Config c;
		REQUIRE(p.apply(&c, &err));
		CHECK_EQ(c.bg.mode, static_cast<int>(BgMode::Transparent));
	}
}