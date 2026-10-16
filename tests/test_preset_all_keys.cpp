#include "core/preset.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

/*
 * Garante que preset.apply() cobre TODAS as chaves de configuracao.
 *
 * Se alguem adicionar um campo ao Config e esquecer de mapear aqui, este teste
 * falha. Os valores abaixo sao todos diferentes dos defaults, entao cada campo
 * "tem que ter mudado".
 */
TEST(preset_cobre_todas_as_chaves)
{
	const std::string text = R"({
        "name": "Todas",
        "settings": {
            "effects_enabled": false,
            "bg_enabled": false,
            "bg_mode": 1,
            "bg_color_a": 111,
            "bg_color_b": 222,
            "bg_padding": 11,
            "bg_corner_radius": 12,
            "bg_shadow": false,
            "bg_shadow_alpha": 0.11,
            "bg_shadow_blur": 13,
            "cursor_enabled": false,
            "cursor_style": 1,
            "cursor_size": 2.5,
            "cursor_shadow": false,
            "cursor_smooth": 0.11,
            "cursor_sway": 0.22,
            "cursor_hide_idle": false,
            "cursor_idle_delay_ms": 333,
            "cursor_loop": true,
            "cursor_loop_ms": 4444,
            "zoom_mode": 2,
            "zoom_amount": 3.3,
            "zoom_duration_ms": 222,
            "zoom_idle_timeout_ms": 555,
            "zoom_pan_amount": 0.77,
            "click_left_effect": 3,
            "click_left_size": 1.7,
            "click_left_color": 999,
            "click_left_duration_ms": 123,
            "click_left_easing": 1,
            "click_right_effect": 2,
            "click_right_size": 0.4,
            "click_right_color": 888,
            "click_right_duration_ms": 321,
            "click_right_easing": 3,
            "click_sound_enabled": false,
            "click_sound_file": "x.wav",
            "click_sound_volume": 0.42,
            "click_sound_left": false,
            "click_sound_right": true,
            "motion_blur_enabled": false,
            "motion_blur_amount": 0.66,
            "spotlight_enabled": true,
            "spotlight_radius": 0.44,
            "spotlight_dim": 0.33,
            "keys_enabled": false,
            "keys_modifiers_only": true,
            "keys_ttl_ms": 1234,
            "keys_max": 9
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
	CHECK_EQ(c.bg.color_a, 111u);
	CHECK_EQ(c.bg.color_b, 222u);
	CHECK_NEAR(c.bg.padding, 11.0, 1e-9);
	CHECK_NEAR(c.bg.corner_radius, 12.0, 1e-9);
	CHECK(!c.bg.shadow);
	CHECK_NEAR(c.bg.shadow_alpha, 0.11, 1e-9);
	CHECK_NEAR(c.bg.shadow_blur, 13.0, 1e-9);

	CHECK(!c.cursor.enabled);
	CHECK_EQ(c.cursor.style, 1);
	CHECK_NEAR(c.cursor.size, 2.5, 1e-9);
	CHECK(!c.cursor.shadow);
	CHECK_NEAR(c.cursor.smooth, 0.11, 1e-9);
	CHECK_NEAR(c.cursor.sway, 0.22, 1e-9);
	CHECK(!c.cursor.hide_idle);
	CHECK_NEAR(c.cursor.idle_delay_ms, 333.0, 1e-9);
	CHECK(c.cursor.loop);
	CHECK_NEAR(c.cursor.loop_ms, 4444.0, 1e-9);

	CHECK_EQ(c.zoom.mode, 2);
	CHECK_NEAR(c.zoom.amount, 3.3, 1e-9);
	CHECK_NEAR(c.zoom.duration_ms, 222.0, 1e-9);
	CHECK_NEAR(c.zoom.idle_timeout_ms, 555.0, 1e-9);
	CHECK_NEAR(c.zoom.pan_amount, 0.77, 1e-9);

	CHECK_EQ(c.click_left.effect, 3);
	CHECK_NEAR(c.click_left.size, 1.7, 1e-9);
	CHECK_EQ(c.click_left.color, 999u);
	CHECK_NEAR(c.click_left.duration_ms, 123.0, 1e-9);
	CHECK_EQ(c.click_left.easing, 1);

	CHECK_EQ(c.click_right.effect, 2);
	CHECK_NEAR(c.click_right.size, 0.4, 1e-9);
	CHECK_EQ(c.click_right.color, 888u);
	CHECK_NEAR(c.click_right.duration_ms, 321.0, 1e-9);
	CHECK_EQ(c.click_right.easing, 3);

	CHECK(!c.click_sound.enabled);
	CHECK_EQ(c.click_sound.file, std::string("x.wav"));
	CHECK_NEAR(c.click_sound.volume, 0.42, 1e-9);
	CHECK(!c.click_sound.left);
	CHECK(c.click_sound.right);

	CHECK(!c.motion_blur.enabled);
	CHECK_NEAR(c.motion_blur.amount, 0.66, 1e-9);

	CHECK(c.spotlight.enabled);
	CHECK_NEAR(c.spotlight.radius, 0.44, 1e-9);
	CHECK_NEAR(c.spotlight.dim, 0.33, 1e-9);

	CHECK(!c.keys.enabled);
	CHECK(c.keys.modifiers_only);
	CHECK_NEAR(c.keys.ttl_ms, 1234.0, 1e-9);
	CHECK_EQ(c.keys.max_keys, 9);
}
