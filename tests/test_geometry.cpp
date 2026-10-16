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

TEST(geometry_clamp_focus)
{
	/* Zoom 1: apenas o centro. */
	CHECK_NEAR(clamp_focus(0.1, 1.0), 0.5, 1e-9);
	/* Zoom 2: margem 0.25 => foco entre 0.25 e 0.75. */
	CHECK_NEAR(clamp_focus(0.0, 2.0), 0.25, 1e-9);
	CHECK_NEAR(clamp_focus(1.0, 2.0), 0.75, 1e-9);
	/* Dentro da faixa nao muda. */
	CHECK_NEAR(clamp_focus(0.5, 2.0), 0.5, 1e-9);
}
