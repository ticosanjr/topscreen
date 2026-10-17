#include "profiles.hpp"

namespace topscreen::core {

Config make_profile_seed(int index)
{
	Config c;

	switch (((index % 4) + 4) % 4) {
	case 1: /* B: verde + laranja, zoom forte */
		c.click_left.color = 0xFF00FF00;  // verde (ABGR)
		c.click_right.color = 0xFF00A5FF; // laranja
		c.zoom.amount = 2.0;
		break;

	case 2: /* C: roxo + ciano, zoom suave, fundo vertical */
		c.click_left.color = 0xFFFF5C7C;  // roxo
		c.click_right.color = 0xFFFFE500; // ciano
		c.zoom.amount = 1.4;
		c.bg.mode = static_cast<int>(BgMode::Vertical);
		break;

	case 3: /* D: amarelo + branco, zoom desligado */
		c.click_left.color = 0xFF00D6FF;  // amarelo
		c.click_right.color = 0xFFFFFFFF; // branco
		c.zoom.mode = static_cast<int>(ZoomMode::Off);
		break;

	case 0: /* A: padrao */
	default:
		break;
	}

	return c;
}

} // namespace topscreen::core
