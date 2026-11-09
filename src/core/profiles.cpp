#include "profiles.hpp"

#include "keys.hpp"

namespace topscreen::core {

int profile_index(int index)
{
	const int n = index % keys::kProfileCount;
	return n < 0 ? n + keys::kProfileCount : n;
}

const char *profile_label(int index)
{
	static const char *const kLabels[keys::kProfileCount] = {
		"Profile A", "Profile B", "Profile C", "Profile D", "Profile E",
		"Profile F", "Profile G", "Profile H", "Profile I", "Profile J",
	};
	return kLabels[profile_index(index)];
}

Config make_profile_seed(int index)
{
	Config c;

	/* The profiles are intentionally usable without saving first:
	 *   A-D: everyday presets (balanced, clean, teaching, streaming)
	 *   E-I: progressively more aggressive looks
	 *   J:   full passthrough (master effects off) */

	switch (profile_index(index)) {
	case 0: /* A: balanced default */
		c.bg.mode = static_cast<int>(BgMode::Gradient);
		c.cursor.style = static_cast<int>(CursorStyle::MacOs);
		c.cursor.size = 1.4;
		c.zoom.mode = static_cast<int>(ZoomMode::Auto);
		c.zoom.amount = 1.35;
		c.zoom.duration_ms = 450.0;
		c.click_left.effect = static_cast<int>(ClickEffect::Ripple);
		c.click_left.color = 0xFFFFFFFF;
		c.click_right.effect = static_cast<int>(ClickEffect::Ring);
		c.click_right.color = 0xFF7C5CFF;
		c.click_right.size = 1.1;
		c.click_sound.volume = 0.60;
		c.keys.enabled = true;
		c.highlight.enabled = true;
		c.highlight.mode = static_cast<int>(HighlightMode::Box);
		c.highlight.color = 0x6600E0FF;
		c.highlight.opacity = 0.35;
		c.highlight.hold_ms = 900.0;
		c.highlight.fade_ms = 300.0;
		break;

	case 1: /* B: clean minimal */
		c.bg.mode = static_cast<int>(BgMode::Transparent);
		c.bg.padding_x = 40.0;
		c.bg.padding_y = 40.0;
		c.bg.corner_radius = 14.0;
		c.bg.shadow_alpha = 0.25;
		c.bg.shadow_blur = 30.0;
		c.bg.shadow_offset = 8.0;
		c.cursor.style = static_cast<int>(CursorStyle::Bibata);
		c.cursor.size = 1.2;
		c.cursor.shadow = false;
		c.cursor.idle_delay_ms = 1500.0;
		c.zoom.mode = static_cast<int>(ZoomMode::Off);
		c.click_left.effect = static_cast<int>(ClickEffect::QuietCircle);
		c.click_left.size = 0.9;
		c.click_left.duration_ms = 450.0;
		c.click_right.effect = static_cast<int>(ClickEffect::QuietCircle);
		c.click_right.size = 0.9;
		c.click_right.duration_ms = 450.0;
		c.click_sound.volume = 0.45;
		c.click_sound.file_right.clear();
		c.click_sound.right = false;
		c.keys.enabled = false;
		c.highlight.enabled = false;
		break;

	case 2: /* C: presentation / teaching */
		c.bg.mode = static_cast<int>(BgMode::Gradient);
		c.bg.color_a = 0xFF322A3A;
		c.bg.color_b = 0xFF6B3B4A;
		c.bg.gradient_angle = 90.0;
		c.bg.padding_x = 80.0;
		c.bg.padding_y = 80.0;
		c.bg.corner_radius = 22.0;
		c.cursor.style = static_cast<int>(CursorStyle::Bibata);
		c.cursor.size = 1.35;
		c.zoom.mode = static_cast<int>(ZoomMode::Auto);
		c.zoom.amount = 1.25;
		c.zoom.duration_ms = 350.0;
		c.zoom.idle_timeout_ms = 1200.0;
		c.click_left.effect = static_cast<int>(ClickEffect::WaterRippleClear);
		c.click_left.size = 1.2;
		c.click_right.effect = static_cast<int>(ClickEffect::Ring);
		c.click_right.size = 1.1;
		c.click_sound.volume = 0.50;
		c.click_sound.file_right.clear();
		c.click_sound.right = false;
		c.keys.enabled = true;
		c.keys.band = true;
		c.keys.pos_x = 0.5;
		c.keys.pos_y = 0.90;
		c.keys.size = 1.15;
		c.keys.ttl_ms = 2500.0;
		c.keys.max_keys = 6;
		c.keys.show_shortcuts = true;
		c.highlight.enabled = true;
		c.highlight.mode = static_cast<int>(HighlightMode::Box);
		c.highlight.color = 0x6600E0FF;
		c.highlight.opacity = 0.45;
		c.highlight.thickness = 0.005;
		c.highlight.hold_ms = 1200.0;
		c.highlight.fade_ms = 300.0;
		break;

	case 3: /* D: streaming / gaming */
		c.bg.mode = static_cast<int>(BgMode::Solid);
		c.bg.color_a = 0xFF14141C;
		c.bg.padding_x = 36.0;
		c.bg.padding_y = 36.0;
		c.bg.corner_radius = 12.0;
		c.bg.shadow_alpha = 0.40;
		c.bg.shadow_blur = 40.0;
		c.bg.shadow_offset = 10.0;
		c.cursor.style = static_cast<int>(CursorStyle::BibataIce);
		c.cursor.size = 1.25;
		c.zoom.mode = static_cast<int>(ZoomMode::Auto);
		c.zoom.amount = 1.60;
		c.zoom.duration_ms = 300.0;
		c.zoom.idle_timeout_ms = 1200.0;
		c.click_left.effect = static_cast<int>(ClickEffect::Flare);
		c.click_left.color = 0xFF00D7FF;
		c.click_left.size = 1.4;
		c.click_left.duration_ms = 500.0;
		c.click_right.effect = static_cast<int>(ClickEffect::Echo);
		c.click_right.color = 0xFFFF5599;
		c.click_right.size = 1.2;
		c.click_right.duration_ms = 600.0;
		c.click_sound.volume = 0.55;
		c.keys.enabled = false;
		c.highlight.enabled = false;
		break;

	case 4: /* E: spotlight focus */
		c.bg.mode = static_cast<int>(BgMode::Gradient);
		c.bg.color_a = 0xFF101018;
		c.bg.color_b = 0xFF3A1A2A;
		c.bg.padding_x = 72.0;
		c.bg.padding_y = 72.0;
		c.bg.corner_radius = 20.0;
		c.bg.shadow_alpha = 0.45;
		c.cursor.style = static_cast<int>(CursorStyle::Neon);
		c.cursor.size = 1.3;
		c.zoom.mode = static_cast<int>(ZoomMode::Auto);
		c.zoom.amount = 1.70;
		c.zoom.duration_ms = 300.0;
		c.click_left.effect = static_cast<int>(ClickEffect::Glow);
		c.click_left.color = 0xFF00D7FF;
		c.click_left.size = 1.5;
		c.click_right.effect = static_cast<int>(ClickEffect::Ring);
		c.click_right.color = 0xFFFF5599;
		c.click_right.size = 1.2;
		c.click_sound.volume = 0.60;
		c.keys.enabled = false;
		c.highlight.enabled = false;
		c.spotlight.enabled = true;
		c.spotlight.shape = static_cast<int>(SpotlightShape::Circle);
		c.spotlight.radius = 0.34;
		c.spotlight.dim = 0.72;
		c.spotlight.border_enabled = true;
		c.spotlight.border_color = 0xFFFFFFFF;
		c.spotlight.border_width = 3.0;
		c.spotlight.outside_mode = 0; // Dim
		c.spotlight.zoom_amount = 1.35;
		c.spotlight.in_ms = 220.0;
		c.spotlight.out_ms = 280.0;
		break;

	case 5: /* F: magnifier lens */
		c.bg.mode = static_cast<int>(BgMode::Gradient);
		c.bg.color_a = 0xFF3A2E1E;
		c.bg.color_b = 0xFF6B4A2A;
		c.bg.padding_x = 80.0;
		c.bg.padding_y = 80.0;
		c.bg.corner_radius = 24.0;
		c.cursor.style = static_cast<int>(CursorStyle::Dot);
		c.cursor.size = 1.25;
		c.zoom.mode = static_cast<int>(ZoomMode::Auto);
		c.zoom.amount = 1.50;
		c.zoom.duration_ms = 350.0;
		c.click_left.effect = static_cast<int>(ClickEffect::Ripple);
		c.click_left.color = 0xFF00D7FF;
		c.click_right.effect = static_cast<int>(ClickEffect::Orb);
		c.click_right.size = 1.0;
		c.click_sound.volume = 0.55;
		c.keys.enabled = false;
		c.highlight.enabled = false;
		c.magnifier.enabled = true;
		c.magnifier.shape = static_cast<int>(SpotlightShape::Circle);
		c.magnifier.radius = 0.30;
		c.magnifier.zoom_amount = 2.2;
		c.magnifier.border_enabled = true;
		c.magnifier.border_color = 0xFFFFFFFF;
		c.magnifier.border_width = 4.0;
		c.magnifier.shadow_enabled = true;
		c.magnifier.shadow_color = 0xFF000000;
		c.magnifier.shadow_alpha = 0.55;
		c.magnifier.shadow_blur = 30.0;
		c.magnifier.shadow_offset_y = 12.0;
		c.magnifier.in_ms = 250.0;
		c.magnifier.out_ms = 300.0;
		break;

	case 6: /* G: neon energy */
		c.bg.mode = static_cast<int>(BgMode::Gradient);
		c.bg.color_a = 0xFFAA00FF;
		c.bg.color_b = 0xFF00DDAA;
		c.bg.gradient_angle = 135.0;
		c.bg.padding_x = 56.0;
		c.bg.padding_y = 56.0;
		c.bg.corner_radius = 26.0;
		c.cursor.style = static_cast<int>(CursorStyle::Neon);
		c.cursor.size = 1.5;
		c.zoom.mode = static_cast<int>(ZoomMode::Auto);
		c.zoom.amount = 2.10;
		c.zoom.duration_ms = 250.0;
		c.zoom.idle_timeout_ms = 1300.0;
		c.click_left.effect = static_cast<int>(ClickEffect::Echo);
		c.click_left.color = 0xFFAA00FF;
		c.click_left.size = 1.6;
		c.click_left.duration_ms = 550.0;
		c.click_right.effect = static_cast<int>(ClickEffect::Flare);
		c.click_right.color = 0xFF00DDAA;
		c.click_right.size = 1.5;
		c.click_right.duration_ms = 500.0;
		c.click_sound.volume = 0.70;
		c.keys.enabled = true;
		c.keys.band = true;
		c.keys.pos_x = 0.5;
		c.keys.pos_y = 0.90;
		c.keys.size = 1.1;
		c.keys.ttl_ms = 2000.0;
		c.keys.max_keys = 8;
		c.highlight.enabled = true;
		c.highlight.mode = static_cast<int>(HighlightMode::Line);
		c.highlight.color = 0x6633CCFF;
		c.highlight.opacity = 0.40;
		c.highlight.thickness = 0.006;
		c.highlight.hold_ms = 900.0;
		c.highlight.fade_ms = 250.0;
		break;

	case 7: /* H: cinematic wallpaper */
		c.bg.mode = static_cast<int>(BgMode::Wallpaper);
		c.bg.image = "wallpapers/fiber.jpg";
		c.bg.image_blur = 2.0;
		c.bg.padding_x = 100.0;
		c.bg.padding_y = 80.0;
		c.bg.corner_radius = 30.0;
		c.bg.shadow_alpha = 0.50;
		c.bg.shadow_blur = 70.0;
		c.bg.shadow_offset = 18.0;
		c.cursor.style = static_cast<int>(CursorStyle::BibataIce);
		c.cursor.size = 1.3;
		c.zoom.mode = static_cast<int>(ZoomMode::Auto);
		c.zoom.amount = 1.25;
		c.zoom.duration_ms = 600.0;
		c.zoom.idle_timeout_ms = 2000.0;
		c.click_left.effect = static_cast<int>(ClickEffect::WaterRipple);
		c.click_left.color = 0xCCFFFFFF;
		c.click_left.size = 1.3;
		c.click_left.duration_ms = 800.0;
		c.click_right.effect = static_cast<int>(ClickEffect::WaterRippleClear);
		c.click_right.size = 1.1;
		c.click_sound.volume = 0.45;
		c.click_sound.file_right.clear();
		c.click_sound.right = false;
		c.keys.enabled = false;
		c.highlight.enabled = false;
		break;

	case 8: /* I: maximum effects */
		c.bg.mode = static_cast<int>(BgMode::Gradient);
		c.bg.color_a = 0xFFFF5599;
		c.bg.color_b = 0xFF00D7FF;
		c.bg.gradient_angle = 45.0;
		c.bg.padding_x = 90.0;
		c.bg.padding_y = 90.0;
		c.bg.corner_radius = 28.0;
		c.bg.shadow_alpha = 0.55;
		c.bg.shadow_blur = 60.0;
		c.bg.shadow_offset = 16.0;
		c.cursor.style = static_cast<int>(CursorStyle::Neon);
		c.cursor.size = 1.6;
		c.zoom.mode = static_cast<int>(ZoomMode::Auto);
		c.zoom.amount = 2.60;
		c.zoom.duration_ms = 220.0;
		c.zoom.idle_timeout_ms = 1800.0;
		c.click_left.effect = static_cast<int>(ClickEffect::Flare);
		c.click_left.color = 0xFFFF9933;
		c.click_left.size = 1.8;
		c.click_left.duration_ms = 450.0;
		c.click_right.effect = static_cast<int>(ClickEffect::Echo);
		c.click_right.color = 0xFF00D7FF;
		c.click_right.size = 1.7;
		c.click_right.duration_ms = 600.0;
		c.click_sound.volume = 0.80;
		c.keys.enabled = true;
		c.keys.band = true;
		c.keys.pos_x = 0.5;
		c.keys.pos_y = 0.90;
		c.keys.size = 1.2;
		c.keys.ttl_ms = 2000.0;
		c.keys.max_keys = 8;
		c.highlight.enabled = true;
		c.highlight.mode = static_cast<int>(HighlightMode::LineZoom);
		c.highlight.color = 0x6633CCFF;
		c.highlight.opacity = 0.45;
		c.highlight.thickness = 0.008;
		c.highlight.hold_ms = 1200.0;
		c.highlight.fade_ms = 300.0;
		c.highlight.zoom_max = 3.0;
		c.highlight.zoom_delay_ms = 800.0;
		c.highlight.zoom_in_ms = 180.0;
		c.highlight.zoom_hold_ms = 500.0;
		c.highlight.zoom_out_ms = 1000.0;
		c.spotlight.enabled = true;
		c.spotlight.shape = static_cast<int>(SpotlightShape::Circle);
		c.spotlight.radius = 0.40;
		c.spotlight.outside_mode = 1; // Blur
		c.spotlight.blur_amount = 24.0;
		c.spotlight.border_enabled = true;
		c.spotlight.border_color = 0xFFFFFFFF;
		c.spotlight.border_width = 3.0;
		c.spotlight.zoom_amount = 1.50;
		c.spotlight.in_ms = 200.0;
		c.spotlight.out_ms = 300.0;
		break;

	case 9: /* J: full passthrough */
		c.effects_enabled = false;
		c.bg.enabled = false;
		c.bg.shadow = false;
		c.cursor.enabled = false;
		c.zoom.mode = static_cast<int>(ZoomMode::Off);
		c.zoom.amount = 1.0;
		c.click_sound.enabled = false;
		c.keys.enabled = false;
		c.highlight.enabled = false;
		c.spotlight.enabled = false;
		c.magnifier.enabled = false;
			break;

	default:
		break;
	}

	return c;
}

} // namespace topscreen::core
