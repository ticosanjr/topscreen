#include "core/highlighter.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(highlight_drag_active)
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
	CHECK_NEAR(r.alpha, 0.35, 1e-9); // default opacity ceiling
}

TEST(highlight_normalized_rectangle)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	m.configure(cfg);

	/* Drags "backwards" (bottom-right corner to top-left). */
	m.begin(0.8, 0.9, 0.0);
	m.move(0.3, 0.2, 10.0);
	m.update(10.0);

	const HighlightRender r = m.render();
	CHECK(r.x0 < r.x1);
	CHECK(r.y0 < r.y1);
	CHECK_NEAR(r.x0, 0.3, 1e-9);
	CHECK_NEAR(r.y1, 0.9, 1e-9);
}

TEST(highlight_hold_and_fade)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.hold_ms = 500.0;
	cfg.fade_ms = 200.0;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 100.0);
	m.end(200.0);

	m.update(400.0); /* within the hold */
	CHECK(m.render().active);
	CHECK_NEAR(m.render().alpha, 0.35, 1e-9);

	m.update(700.0); /* end of the hold */
	CHECK(m.render().active);

	m.update(850.0); /* middle of the fade */
	CHECK(m.render().active);
	CHECK(m.render().alpha < 1.0);
	CHECK(m.render().alpha > 0.0);

	m.update(1000.0); /* past the fade */
	CHECK(!m.render().active);
}

TEST(highlight_small_drag_ignored)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.min_span = 0.05;
	m.configure(cfg);

	m.begin(0.5, 0.5, 0.0);
	m.move(0.51, 0.51, 10.0); /* practically a click */
	m.update(10.0);
	CHECK(!m.render().active);
}

TEST(highlight_disabled)
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

TEST(highlight_opacity_scales_alpha)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.opacity = 0.5;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 100.0);
	m.update(100.0);

	const HighlightRender r = m.render();
	CHECK(r.active);
	CHECK_NEAR(r.alpha, 0.5, 1e-9);
}

TEST(highlight_opacity_zero_hides)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.opacity = 0.0;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 100.0);
	m.update(100.0);

	CHECK(m.render().active);
	CHECK_NEAR(m.render().alpha, 0.0, 1e-9);
}

TEST(highlight_opacity_one_keeps_full_strength)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.opacity = 1.0;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 100.0);
	m.update(100.0);

	const HighlightRender r = m.render();
	CHECK(r.active);
	CHECK_NEAR(r.alpha, 1.0, 1e-9);
}

TEST(highlight_opacity_clamps_above_one)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.opacity = 2.0;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 100.0);
	m.update(100.0);

	CHECK_NEAR(m.render().alpha, 1.0, 1e-9);
}
