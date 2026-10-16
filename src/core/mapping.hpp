#pragma once

#include "types.hpp"

namespace topscreen::core {

/**
 * Area (em pixels de saida) onde a tela capturada e desenhada.
 *
 * Determinada pelo padding (x/y) e, opcionalmente, por "manter proporcao".
 */
struct ContentArea {
	Vec2f origin; // canto superior esquerdo
	Vec2f size;   // largura/altura
};

ContentArea content_area(double target_w, double target_h, double pad_x, double pad_y, bool keep_aspect);

/**
 * Posicao EXIBIDA (content uv) de um ponto da FONTE (source uv).
 *
 * Com zoom `z` e foco `f`, um ponto `s` da fonte aparece em `f + (s - f) * z`
 * (zoom "para frente": o que esta longe do foco se afasta dele).
 *
 * Isto e o inverso da amostragem do shader (`u = f + (c - f) / z`).
 */
Vec2f source_to_display(double s_x, double s_y, double focus_x, double focus_y, double zoom);

/** Pixel de saida de um ponto da fonte (fonte -> exibicao -> pixel). */
Vec2f source_to_pixel(double s_x, double s_y, double focus_x, double focus_y, double zoom,
		      const ContentArea &area);

} // namespace topscreen::core
