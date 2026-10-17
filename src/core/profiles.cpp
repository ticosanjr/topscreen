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
	static const char *const kLabels[keys::kProfileCount] = {"Perfil A", "Perfil B", "Perfil C",
								 "Perfil D"};
	return kLabels[profile_index(index)];
}

Config make_profile_seed(int index)
{
	Config c;

	switch (profile_index(index)) {
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
