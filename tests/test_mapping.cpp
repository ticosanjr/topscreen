#include "core/geometry.hpp"
#include "core/mapping.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(mapping_area_sem_padding)
{
	const ContentArea a = content_area(1920, 1080, 0, 0, false);
	CHECK_NEAR(a.origin.x, 0.0, 1e-9);
	CHECK_NEAR(a.origin.y, 0.0, 1e-9);
	CHECK_NEAR(a.size.x, 1920.0, 1e-9);
	CHECK_NEAR(a.size.y, 1080.0, 1e-9);
}

TEST(mapping_area_com_padding)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, false);
	CHECK_NEAR(a.origin.x, 64.0, 1e-9);
	CHECK_NEAR(a.origin.y, 64.0, 1e-9);
	CHECK_NEAR(a.size.x, 1792.0, 1e-9);
	CHECK_NEAR(a.size.y, 952.0, 1e-9);
}

TEST(mapping_area_manter_proporcao)
{
	/* 1920x1080 com padding 64/64: sc = min(1792/1920, 952/1080) = 0.88148... */
	const ContentArea a = content_area(1920, 1080, 64, 64, true);
	const double sc = 952.0 / 1080.0;
	CHECK_NEAR(a.size.x, 1920.0 * sc, 1e-6);
	CHECK_NEAR(a.size.y, 1080.0 * sc, 1e-6);
	CHECK_NEAR(a.origin.x, (1920.0 - 1920.0 * sc) * 0.5, 1e-6);
	CHECK_NEAR(a.origin.y, (1080.0 - 1080.0 * sc) * 0.5, 1e-6);
}

TEST(mapping_zoom_identidade)
{
	const Vec2f d = source_to_display(0.3, 0.7, 0.5, 0.5, 1.0);
	CHECK_NEAR(d.x, 0.3, 1e-9);
	CHECK_NEAR(d.y, 0.7, 1e-9);
}

TEST(mapping_zoom_para_frente)
{
	/* Com foco 0.5 e zoom 2, um ponto 0.1 fora do foco aparece 0.2 fora. */
	const Vec2f d = source_to_display(0.6, 0.4, 0.5, 0.5, 2.0);
	CHECK_NEAR(d.x, 0.7, 1e-9);
	CHECK_NEAR(d.y, 0.3, 1e-9);

	/* E o inverso da amostragem do shader: u = f + (d - f)/z. */
	const double u = 0.5 + (d.x - 0.5) / 2.0;
	CHECK_NEAR(u, 0.6, 1e-9);
}

TEST(mapping_fonte_para_pixel)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, false);
	/* centro da fonte -> centro do conteudo */
	const Vec2f c = source_to_pixel(0.5, 0.5, 0.5, 0.5, 1.0, a);
	CHECK_NEAR(c.x, 64.0 + 0.5 * 1792.0, 1e-6);
	CHECK_NEAR(c.y, 64.0 + 0.5 * 952.0, 1e-6);

	/* canto superior esquerdo da fonte -> canto do conteudo */
	const Vec2f tl = source_to_pixel(0.0, 0.0, 0.5, 0.5, 1.0, a);
	CHECK_NEAR(tl.x, 64.0, 1e-6);
	CHECK_NEAR(tl.y, 64.0, 1e-6);
}

/*
 * Os cantos: com o foco ajustado para manter o ponto visivel, um ponto da
 * BORDA tem que aparecer dentro da area de conteudo (nao some da tela).
 */
TEST(mapping_canto_visivel_com_foco_ajustado)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, true);
	const double pontos[] = {0.0, 0.25, 0.5, 0.75, 1.0};
	const double zooms[] = {1.6, 2.5};

	for (double s : pontos) {
		for (double z : zooms) {
			/* pior caso: foco centralizado (pan 0) -> o clamp tem que puxar */
			const double f = clamp_focus_keep(s, 0.5, z);
			const Vec2f d = source_to_display(s, s, f, f, z);
			CHECK(d.x >= -1e-9 && d.x <= 1.0 + 1e-9);
			CHECK(d.y >= -1e-9 && d.y <= 1.0 + 1e-9);

			const Vec2f p = source_to_pixel(s, s, f, f, z, a);
			CHECK(p.x >= a.origin.x - 1e-6);
			CHECK(p.x <= a.origin.x + a.size.x + 1e-6);
			CHECK(p.y >= a.origin.y - 1e-6);
			CHECK(p.y <= a.origin.y + a.size.y + 1e-6);
		}
	}
}
TEST(mapping_consistencia_com_shader)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, true);
	const double f = 0.42;
	const double z = 1.8;

	for (int i = 1; i < 10; ++i) {
		for (int j = 1; j < 10; ++j) {
			const double cx = i / 10.0;
			const double cy = j / 10.0;

			/* o que o shader amostra para este pixel */
			const double ux = f + (cx - f) / z;
			const double uy = f + (cy - f) / z;

			/* o overlay naquele ponto da fonte deve cair no MESMO pixel */
			const Vec2f p = source_to_pixel(ux, uy, f, f, z, a);
			CHECK_NEAR(p.x, a.origin.x + cx * a.size.x, 1e-6);
			CHECK_NEAR(p.y, a.origin.y + cy * a.size.y, 1e-6);
		}
	}
}
