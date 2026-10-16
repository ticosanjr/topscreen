#include "core/highlighter.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(highlight_arrasto_ativo)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	m.configure(cfg);

	m.begin(0.2, 0.3, 0.0);
	m.move(0.6, 0.7, 100.0);
	m.update(100.0);

	const HighlightRender r = m.render();
	CHECK(r.active);
	CHECK_NEAR(r.x0, 0.2, 1e-9);
	CHECK_NEAR(r.y0, 0.3, 1e-9);
	CHECK_NEAR(r.x1, 0.6, 1e-9);
	CHECK_NEAR(r.y1, 0.7, 1e-9);
	CHECK_NEAR(r.alpha, 1.0, 1e-9);
}

TEST(highlight_retangulo_normalizado)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	m.configure(cfg);

	/* Arrasta "para tras" (canto inferior direito para o superior esquerdo). */
	m.begin(0.8, 0.9, 0.0);
	m.move(0.3, 0.2, 10.0);
	m.update(10.0);

	const HighlightRender r = m.render();
	CHECK(r.x0 < r.x1);
	CHECK(r.y0 < r.y1);
	CHECK_NEAR(r.x0, 0.3, 1e-9);
	CHECK_NEAR(r.y1, 0.9, 1e-9);
}

TEST(highlight_hold_e_fade)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.hold_ms = 500.0;
	cfg.fade_ms = 200.0;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 100.0);
	m.end(200.0);

	m.update(400.0); /* dentro do hold */
	CHECK(m.render().active);
	CHECK_NEAR(m.render().alpha, 1.0, 1e-9);

	m.update(700.0); /* fim do hold */
	CHECK(m.render().active);

	m.update(850.0); /* meio do fade */
	CHECK(m.render().active);
	CHECK(m.render().alpha < 1.0);
	CHECK(m.render().alpha > 0.0);

	m.update(1000.0); /* passou o fade */
	CHECK(!m.render().active);
}

TEST(highlight_arrasto_pequeno_ignorado)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.min_span = 0.05;
	m.configure(cfg);

	m.begin(0.5, 0.5, 0.0);
	m.move(0.51, 0.51, 10.0); /* praticamente um clique */
	m.update(10.0);
	CHECK(!m.render().active);
}

TEST(highlight_desabilitado)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.enabled = false;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 10.0);
	m.update(10.0);
	CHECK(!m.render().active);
}
