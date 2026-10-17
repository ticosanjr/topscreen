#include "core/geometry.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(geometry_sd_round_box_sinal)
{
	/* Centro: negativo (dentro). Fora: positivo. */
	CHECK(sd_round_box(0.0, 0.0, 100.0, 50.0, 10.0) < 0.0);
	CHECK(sd_round_box(200.0, 0.0, 100.0, 50.0, 10.0) > 0.0);
	/* Borda reta: ~0. */
	CHECK_NEAR(sd_round_box(100.0, 0.0, 100.0, 50.0, 0.0), 0.0, 1e-9);
	/* Canto arredondado: o ponto (100,40) esta na borda do raio 10. */
	CHECK_NEAR(sd_round_box(100.0, 40.0, 100.0, 50.0, 10.0), 0.0, 1e-9);
	/* Um ponto no canto "quadrado" fica fora do retangulo arredondado. */
	CHECK(sd_round_box(100.0, 50.0, 100.0, 50.0, 10.0) > 0.0);
}

TEST(geometry_mask_centro_e_fora)
{
	CHECK_NEAR(rounded_rect_mask(0.0, 0.0, 100.0, 50.0, 10.0, 1.0), 1.0, 1e-6);
	CHECK_NEAR(rounded_rect_mask(500.0, 500.0, 100.0, 50.0, 10.0, 1.0), 0.0, 1e-6);
	/* No canto quadrado (fora do raio) deve ser 0. */
	CHECK_NEAR(rounded_rect_mask(99.0, 49.0, 100.0, 50.0, 10.0, 1.0), 0.0, 1e-3);
}

TEST(geometry_zoom_uv_identidade)
{
	const Vec2f uv = zoom_uv(0.3, 0.7, 0.5, 0.5, 1.0);
	CHECK_NEAR(uv.x, 0.3, 1e-9);
	CHECK_NEAR(uv.y, 0.7, 1e-9);
}

TEST(geometry_zoom_uv_amplia_em_torno_do_foco)
{
	/* Com foco em 0.5 e zoom 2, a borda (0) mapeia para 0.25. */
	const Vec2f uv = zoom_uv(0.0, 1.0, 0.5, 0.5, 2.0);
	CHECK_NEAR(uv.x, 0.25, 1e-9);
	CHECK_NEAR(uv.y, 0.75, 1e-9);
	/* O foco permanece no foco. */
	const Vec2f f = zoom_uv(0.5, 0.5, 0.5, 0.5, 3.0);
	CHECK_NEAR(f.x, 0.5, 1e-9);
}

TEST(geometry_pixel_to_content)
{
	const Vec2f c = pixel_to_content(50.0, 50.0, 1100.0, 700.0, 50.0);
	CHECK_NEAR(c.x, 0.0, 1e-9);
	CHECK_NEAR(c.y, 0.0, 1e-9);

	const Vec2f mid = pixel_to_content(550.0, 350.0, 1100.0, 700.0, 50.0);
	CHECK_NEAR(mid.x, 0.5, 1e-9);
	CHECK_NEAR(mid.y, 0.5, 1e-9);
}

TEST(geometry_center_focus)
{
	/* Centro da fonte com qualquer zoom -> foco 0.5 (cursor no centro). */
	CHECK_NEAR(center_focus_for(0.5, 2.0), 0.5, 1e-9);

	/* Bordas -> foco no limite (0/1). */
	CHECK_NEAR(center_focus_for(0.0, 1.6), 0.0, 1e-9);
	CHECK_NEAR(center_focus_for(1.0, 1.6), 1.0, 1e-9);

	/*
	 * Antecipacao: com zoom 1.6, o cursor em s=0.3125 ja poe a vista na
	 * borda esquerda (foco 0) -> o canto aparece ANTES de o mouse chegar.
	 */
	CHECK_NEAR(center_focus_for(0.3125, 1.6), 0.0, 1e-6);
	CHECK(center_focus_for(0.6, 1.6) > 0.0);

	/* Sem zoom o foco nao importa. */
	CHECK_NEAR(center_focus_for(0.0, 1.0), 0.5, 1e-9);
}

TEST(geometry_center_focus_mantem_cursor_visivel)
{
	for (double z : {1.3, 1.6, 2.0, 3.0}) {
		for (double s = 0.0; s <= 1.0; s += 0.05) {
			const double f = center_focus_for(s, z);
			const double c = f + (s - f) * z;
			CHECK(c >= -1e-9 && c <= 1.0 + 1e-9);
		}
	}
}
TEST(geometry_clamp_focus_cantos)
{
	/* Foco em [0,1]: os cantos sao alcancaveis (a janela 1/zoom cabe na fonte). */
	CHECK_NEAR(clamp_focus(0.0, 2.0), 0.0, 1e-9);
	CHECK_NEAR(clamp_focus(1.0, 2.0), 1.0, 1e-9);
	CHECK_NEAR(clamp_focus(-0.5, 2.0), 0.0, 1e-9);
	CHECK_NEAR(clamp_focus(1.5, 2.0), 1.0, 1e-9);
	CHECK_NEAR(clamp_focus(0.5, 1.0), 0.5, 1e-9);
}

TEST(geometry_focus_limits_mantem_ponto_visivel)
{
	/* Ponto no canto esquerdo: o foco tem que ir a 0 para ele aparecer. */
	FocusLimits l = focus_limits(0.0, 2.0);
	CHECK_NEAR(l.lo, 0.0, 1e-9);
	CHECK_NEAR(l.hi, 0.0, 1e-9);
	CHECK_NEAR(clamp_focus_keep(0.0, 0.8, 2.0), 0.0, 1e-9);

	/* Ponto no canto direito: foco 1. */
	l = focus_limits(1.0, 2.0);
	CHECK_NEAR(l.lo, 1.0, 1e-9);
	CHECK_NEAR(l.hi, 1.0, 1e-9);

	/* No centro a faixa permite qualquer foco (pan livre). */
	l = focus_limits(0.5, 2.0);
	CHECK(l.lo <= 1e-9);
	CHECK(l.hi >= 1.0 - 1e-9);

	/* Sem zoom: qualquer foco serve. */
	l = focus_limits(0.0, 1.0);
	CHECK_NEAR(l.lo, 0.0, 1e-9);
	CHECK_NEAR(l.hi, 1.0, 1e-9);
}
