#pragma once

#include "types.hpp"

namespace topscreen::core {

/** smoothstep em C++ (espelha o do shader). */
inline double smoothstep(double edge0, double edge1, double x)
{
	if (edge0 == edge1)
		return x < edge0 ? 0.0 : 1.0;
	double t = (x - edge0) / (edge1 - edge0);
	if (t < 0.0)
		t = 0.0;
	if (t > 1.0)
		t = 1.0;
	return t * t * (3.0 - 2.0 * t);
}

/**
 * Distancia assinada a um retangulo arredondado centrado na origem.
 * Negativo dentro, positivo fora. Espelha sd_round_box() do shader.
 */
double sd_round_box(double px, double py, double hx, double hy, double radius);

/**
 * Mascara de cobertura de um retangulo arredondado (0 fora, 1 dentro),
 * com anti-aliasing numa banda de largura `aa` em torno da borda.
 * Espelha o calculo do shader para permitir testes.
 */
double rounded_rect_mask(double px, double py, double hx, double hy, double radius, double aa);

/**
 * Transformacao de UV do zoom/pan.
 * content_uv em 0..1 -> UV de amostragem da tela (0..1).
 */
Vec2f zoom_uv(double content_u, double content_v, double focus_x, double focus_y, double zoom);

/**
 * Converte um pixel de saida em coordenada de conteudo (0..1), considerando
 * o padding interno. Fora do conteudo gera valores fora de 0..1.
 */
Vec2f pixel_to_content(double px, double py, double target_w, double target_h, double pad);

/**
 * Limita o foco do zoom para nao amostrar fora da tela (0..1).
 *
 * Com foco em [0,1] a janela visivel (largura 1/zoom) sempre cabe na fonte.
 */
double clamp_focus(double focus, double zoom);

/** Faixa de foco que mantem um ponto `source_pos` visivel (0..1). */
struct FocusLimits {
	double lo = 0.0;
	double hi = 1.0;
};

FocusLimits focus_limits(double source_pos, double zoom);

/** Limita o foco mantendo o ponto `source_pos` dentro da tela. */
double clamp_focus_keep(double source_pos, double focus, double zoom);

/**
 * Foco que coloca o ponto da fonte `source_pos` no CENTRO da tela.
 *
 * Resolve `0.5 = f + (s - f) * z` para `f`, com resultado em [0,1]:
 *   f = (s*z - 0.5) / (z - 1)
 *
 * Com isso a vista antecipa as bordas (o canto aparece antes de o mouse
 * chegar nele) e o cursor fica visivel o tempo todo.
 */
double center_focus_for(double source_pos, double zoom);

} // namespace topscreen::core
