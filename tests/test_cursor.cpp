#include "core/cursor.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

namespace {
void advance(CursorModel &m, double &t, double seconds, double dt = 1.0 / 60.0)
{
	for (double acc = 0.0; acc < seconds; acc += dt) {
		t += dt * 1000.0;
		m.update(t);
	}
}
} // namespace

TEST(cursor_posicao_inicial)
{
	CursorModel m;
	CursorConfig cfg;
	m.configure(cfg);
	m.reset(0.2, 0.3, 0.0);
	const CursorRender r = m.render();
	CHECK_NEAR(r.x, 0.2, 1e-9);
	CHECK_NEAR(r.y, 0.3, 1e-9);
	CHECK_NEAR(r.size, cfg.size, 1e-9);
}

TEST(cursor_persegue_alvo)
{
	CursorModel m;
	CursorConfig cfg;
	m.configure(cfg);
	m.reset(0.0, 0.0, 0.0);
	m.set_target(1.0, 1.0, 0.0);
	double t = 0.0;
	advance(m, t, 2.0);
	const CursorRender r = m.render();
	CHECK_NEAR(r.x, 1.0, 1e-2);
	CHECK_NEAR(r.y, 1.0, 1e-2);
}

TEST(cursor_idle_reduz_opacidade)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.hide_idle = true;
	cfg.idle_delay_ms = 1000.0;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);

	double t = 0.0;
	advance(m, t, 0.5); /* antes do delay */
	CHECK(m.render().opacity > 0.99);

	advance(m, t, 2.0); /* bem apos delay + fade */
	CHECK(m.render().opacity < 0.01);
	CHECK(!m.visible());
}

TEST(cursor_sem_hide_idle_mantem_opacidade)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.hide_idle = false;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 5.0);
	CHECK_NEAR(m.render().opacity, 1.0, 1e-9);
}

TEST(cursor_loop_volta_ao_inicio)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.loop = true;
	cfg.loop_ms = 1000.0;
	cfg.hide_idle = false;
	m.configure(cfg);
	m.reset(0.1, 0.1, 0.0);

	m.set_target(0.9, 0.9, 0.0);
	double t = 0.0;
	advance(m, t, 1.0);
	CHECK(m.render().x > 0.5); /* ja se moveu para longe */

	/* Fica parado: apos loop_ms deve voltar para perto do inicio. */
	advance(m, t, 4.0);
	CHECK_NEAR(m.render().x, 0.1, 0.05);
	CHECK_NEAR(m.render().y, 0.1, 0.05);
}

TEST(cursor_desabilitado_invisivel)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.enabled = false;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);
	CHECK(!m.render().visible);
	CHECK_NEAR(m.render().opacity, 0.0, 1e-9);
}
