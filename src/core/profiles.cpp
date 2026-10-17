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
	static const char *const kLabels[keys::kProfileCount] = {"Profile A", "Profile B", "Profile C",
								 "Profile D"};
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

	case 2: /* C: purple + cyan, echo halo, soft zoom, vertical background */
		c.click_left.color = 0xFFFF5C7C;  // purple
		c.click_left.effect = static_cast<int>(ClickEffect::Echo);
		c.click_right.color = 0xFFFFE500; // cyan
		c.click_right.effect = static_cast<int>(ClickEffect::Glow);
		c.zoom.amount = 1.4;
		c.bg.mode = static_cast<int>(BgMode::Vertical);
		break;

	case 3: /* D: yellow + white, zoom off */
		c.click_left.color = 0xFF00D6FF;  // yellow
		c.click_right.color = 0xFFFFFFFF; // white
		c.zoom.mode = static_cast<int>(ZoomMode::Off);
		break;

	case 0: /* A: default */
	default:
		break;
	}

	return c;
}

} // namespace topscreen::core
