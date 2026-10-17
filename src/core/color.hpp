#pragma once

#include "types.hpp"

#include <cstdint>

namespace topscreen::core {

/**
 * Conversao de cor.
 *
 * O OBS armazena cores como inteiro no formato 0xAABBGGRR (byte baixo = R).
 */
inline Rgba unpack_abgr(uint32_t c)
{
	Rgba out;
	out.r = static_cast<float>(c & 0xFFu) / 255.0f;
	out.g = static_cast<float>((c >> 8) & 0xFFu) / 255.0f;
	out.b = static_cast<float>((c >> 16) & 0xFFu) / 255.0f;
	out.a = static_cast<float>((c >> 24) & 0xFFu) / 255.0f;
	return out;
}

inline uint32_t pack_abgr(const Rgba &c)
{
	auto q = [](float v) -> uint32_t {
		if (v < 0.0f)
			v = 0.0f;
		if (v > 1.0f)
			v = 1.0f;
		return static_cast<uint32_t>(v * 255.0f + 0.5f);
	};
	return q(c.r) | (q(c.g) << 8) | (q(c.b) << 16) | (q(c.a) << 24);
}

/** Interpolacao linear entre duas cores (t recortado em 0..1). */
inline Rgba lerp_color(const Rgba &a, const Rgba &b, float t)
{
	if (t < 0.0f)
		t = 0.0f;
	if (t > 1.0f)
		t = 1.0f;
	Rgba out;
	out.r = a.r + (b.r - a.r) * t;
	out.g = a.g + (b.g - a.g) * t;
	out.b = a.b + (b.b - a.b) * t;
	out.a = a.a + (b.a - a.a) * t;
	return out;
}

/**
 * Converte a cor do obs_data/propriedades (0xAABBGGRR) para o formato que
 * `gs_effect_set_color` espera (0xAARRGGBB) - ou seja, troca R e B.
 *
 * Sem isso, vermelho aparece como azul na tela.
 */
inline uint32_t abgr_to_argb(uint32_t v)
{
	return (v & 0xFF00FF00u) | ((v & 0x000000FFu) << 16) | ((v >> 16) & 0x000000FFu);
}

} // namespace topscreen::core
