#include "core/zoom.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

namespace {
void advance(ZoomModel &m, double &t, double seconds, double dt = 1.0 / 60.0)
{
	for (double acc = 0.0; acc < seconds; acc += dt) {
		t += dt * 1000.0;
		m.update(t);
	}
}
} // namespace

TEST(zoom_manual_step_e_limites)
{
	ZoomModel m;
	ZoomConfig cfg;
	m.configure(cfg);
	m.reset(0.0);

	CHECK_NEAR(m.manual(), 1.0, 1e-9);
	m.manual_step(1.25);
	CHECK_NEAR(m.manual(), 1.25, 1e-9);
	m.manual_step(0.8);
	CHECK_NEAR(m.manual(), 1.0, 1e-9); /* nunca abaixo de 1 */
	for (int i = 0; i < 40; ++i)
		m.manual_step(2.0);
	CHECK(m.manual() <= 8.0 + 1e-9);
}

TEST(zoom_auto_aproxima_com_foco_recente)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 2.0;
	cfg.duration_ms = 200.0;
	cfg.idle_timeout_ms = 100000.0; /* nao expira durante o teste */
	cfg.pan_amount = 1.0;
	m.configure(cfg);
	m.reset(0.0);

	m.focus_on(0.5, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 1.0);
	CHECK(m.render().zoom > 1.5);
}

TEST(zoom_auto_volta_ao_normal_no_idle)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 2.0;
	cfg.duration_ms = 150.0;
	cfg.idle_timeout_ms = 500.0;
	m.configure(cfg);
	m.reset(0.0);

	m.focus_on(0.5, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 0.4);
	CHECK(m.render().zoom > 1.2);

	/* Passa o idle_timeout: deve retornar para 1.0. */
	advance(m, t, 2.0);
	CHECK_NEAR(m.render().zoom, 1.0, 0.02);
}

TEST(zoom_modo_off_nao_aproxima)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Off);
	m.configure(cfg);
	m.reset(0.0);
	m.focus_on(0.5, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 1.0);
	CHECK_NEAR(m.render().zoom, 1.0, 1e-3);
}

TEST(zoom_centraliza_o_cursor)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 2.0;
	cfg.duration_ms = 100.0;
	cfg.idle_timeout_ms = 100000.0;
	cfg.center_amount = 1.0;
	m.configure(cfg);
	m.reset(0.0);

	m.focus_on(0.5, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 1.0);

	const ZoomRender r = m.render();
	const double c = r.focus_x + (0.5 - r.focus_x) * r.zoom;
	CHECK_NEAR(c, 0.5, 0.02); /* cursor no centro */
}

TEST(zoom_antecipa_o_canto)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 1.6;
	cfg.duration_ms = 100.0;
	cfg.idle_timeout_ms = 100000.0;
	cfg.center_amount = 1.0;
	m.configure(cfg);
	m.reset(0.0);

	/* Cursor a 20% da esquerda: a vista ja tem que estar na borda. */
	m.focus_on(0.2, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 1.5);

	const ZoomRender r = m.render();
	CHECK(r.focus_x < 0.05);

	/* No pixel mais a esquerda, a fonte amostrada e o canto (0). */
	const double u_no_pixel_0 = r.focus_x + (0.0 - r.focus_x) / r.zoom;
	CHECK(u_no_pixel_0 <= 0.02);
}
TEST(zoom_foco_alcanca_os_cantos)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 2.0;
	cfg.duration_ms = 100.0;
	cfg.idle_timeout_ms = 100000.0;
	cfg.pan_amount = 1.0;
	m.configure(cfg);
	m.reset(0.0);

	/* Canto inferior direito da fonte. */
	m.focus_on(1.0, 1.0, 0.0);
	double t = 0.0;
	advance(m, t, 1.0);
	const ZoomRender r = m.render();
	CHECK(r.focus_x > 0.9);
	CHECK(r.focus_y > 0.9);

	/* O ponto da fonte (1,1) tem que aparecer DENTRO da tela. */
	const double d = r.focus_x + (1.0 - r.focus_x) * r.zoom;
	CHECK(d <= 1.0 + 1e-6);
}

TEST(zoom_canto_esquerdo_com_pan_pequeno)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 1.6;
	cfg.duration_ms = 100.0;
	cfg.idle_timeout_ms = 100000.0;
	cfg.pan_amount = 0.25; /* pan "suave": mesmo assim o canto deve ser alcancado */
	m.configure(cfg);
	m.reset(0.0);

	m.focus_on(0.0, 0.0, 0.0);
	double t = 0.0;
	advance(m, t, 1.5);

	const ZoomRender r = m.render();
	/* o foco tem que ser puxado para perto de 0 (nao ficar em 0.375) */
	CHECK(r.focus_x < 0.1);
	CHECK(r.focus_y < 0.1);

	/* e o ponto (0,0) tem que estar visivel (0 <= d <= 1) */
	const double d = r.focus_x + (0.0 - r.focus_x) * r.zoom;
	CHECK(d >= -1e-6);
	CHECK(d <= 1.0 + 1e-6);
}

TEST(zoom_pan_nao_decai_com_o_tempo)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 2.0;
	cfg.duration_ms = 100.0;
	cfg.idle_timeout_ms = 100000.0;
	cfg.pan_amount = 0.5;
	m.configure(cfg);
	m.reset(0.0);

	m.focus_on(0.8, 0.8, 0.0);
	double t = 0.0;
	advance(m, t, 1.0);
	const double fx1 = m.render().focus_x;

	/* muito tempo depois, o foco nao pode ter "decaido" para o centro */
	advance(m, t, 3.0);
	const double fx2 = m.render().focus_x;
	CHECK_NEAR(fx1, fx2, 1e-6);
	CHECK(fx2 > 0.6);
}
