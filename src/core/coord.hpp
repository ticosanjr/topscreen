#pragma once

#include "types.hpp"

namespace topscreen::core {

/** Retangulo de um monitor/tela em coordenadas globais. */
struct Monitor {
	double x = 0.0;
	double y = 0.0;
	double width = 1920.0;
	double height = 1080.0;
};

/** Converte um ponto global de tela para coordenada de conteudo (0..1). */
Vec2f screen_to_content(double sx, double sy, const Monitor &m);

/** Converte coordenada de conteudo (0..1) para um ponto global de tela. */
Vec2f content_to_screen(double cu, double cv, const Monitor &m);

/** true se o ponto global pertence ao monitor. */
bool monitor_contains(const Monitor &m, double sx, double sy);

} // namespace topscreen::core
