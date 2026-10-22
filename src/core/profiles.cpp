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

	switch (profile_index(index)) {
	case 1: /* B: green + orange, strong zoom */
		c.click_left.color = 0xFF00FF00;  // green (ABGR)
		c.click_right.color = 0xFF00A5FF; // orange
		c.zoom.amount = 2.0;
		break;

	case 2: /* C: purple + cyan, echo halo, soft zoom, vertical gradient */
		c.click_left.color = 0xFFFF5C7C;  // purple
		c.click_left.effect = static_cast<int>(ClickEffect::Echo);
		c.click_right.color = 0xFFFFE500; // cyan
		c.click_right.effect = static_cast<int>(ClickEffect::Glow);
		c.zoom.amount = 1.4;
		c.bg.mode = static_cast<int>(BgMode::Gradient);
		c.bg.gradient_angle = 90.0;
		break;

	case 3: /* D: yellow + white, zoom off */
		c.click_left.color = 0xFF00D6FF;  // yellow
		c.click_right.color = 0xFFFFFFFF; // white
		c.zoom.mode = static_cast<int>(ZoomMode::Off);
		break;

	case 4: /* E: blue + orange, solid background */
		c.click_left.color = 0xFFFF9933;  // blue
		c.click_right.color = 0xFF0099FF; // orange
		c.zoom.amount = 1.8;
		c.bg.mode = static_cast<int>(BgMode::Solid);
		break;

	case 5: /* F: magenta + lime, strong zoom, vertical gradient */
		c.click_left.color = 0xFFAA00FF;  // magenta
		c.click_right.color = 0xFF00DDAA; // lime
		c.zoom.amount = 2.5;
		c.bg.mode = static_cast<int>(BgMode::Gradient);
		c.bg.gradient_angle = 90.0;
		break;

	case 6: /* G: white + pink, gentle zoom, green highlighter */
		c.click_left.color = 0xFFFFFFFF;  // white
		c.click_right.color = 0xFFCC88FF; // pink
		c.zoom.amount = 1.25;
		c.highlight.color = 0x6633CCFF;   // green highlighter
		break;

	case 7: /* H: teal + red, very strong zoom, no click sound */
		c.click_left.color = 0xFFCCCC00;  // teal
		c.click_right.color = 0xFF2222CC; // red
		c.zoom.amount = 3.0;
		c.click_sound.enabled = false;
		break;

	case 8: /* I: violet + gold, neon cursor, manual zoom */
		c.click_left.color = 0xFFFF5599;  // violet
		c.click_right.color = 0xFF00D7FF; // gold
		c.cursor.style = static_cast<int>(CursorStyle::Neon);
		c.zoom.mode = static_cast<int>(ZoomMode::Manual);
		c.zoom.amount = 1.6;
		break;

	case 9: /* J: green + black, wallpaper background, dot cursor */
		c.click_left.color = 0xFF66CC22;  // green
		c.click_right.color = 0xFF111111; // near black
		c.cursor.style = static_cast<int>(CursorStyle::Dot);
		c.bg.image = "wallpapers/fiber.jpg";
		c.bg.mode = static_cast<int>(BgMode::Wallpaper);
		break;

	case 0: /* A: default */
	default:
		break;
	}

	return c;
}

} // namespace topscreen::core
