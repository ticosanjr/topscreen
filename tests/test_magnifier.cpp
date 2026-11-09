#include "core/magnifier.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(magnifier_starts_off_and_animates_in)
{
	MagnifierConfig cfg;
	cfg.enabled = false;
	cfg.in_ms = 100.0;
	cfg.out_ms = 100.0;
	cfg.zoom_amount = 2.0;

	MagnifierModel m;
	m.configure(cfg);
	CHECK(!m.active());
	CHECK_NEAR(m.render().amount, 0.0, 1e-9);
	CHECK_NEAR(m.render().zoom, 1.0, 1e-9);

	m.set_active(true, 0.0);
	m.update(50.0, 0.25, 0.75);
	const MagnifierRender half = m.render();
	CHECK(half.amount > 0.0 && half.amount < 1.0);
	CHECK(half.zoom > 1.0 && half.zoom < 2.0);
	/* The view follows the cursor; the canvas stops at the lens radius. */
	CHECK_NEAR(half.sample_x, 0.25, 1e-9);
	CHECK_NEAR(half.sample_y, 0.75, 1e-9);
	CHECK_NEAR(half.focus_x, 0.28, 1e-9);
	CHECK_NEAR(half.focus_y, 0.72, 1e-9);

	m.update(100.0, 0.25, 0.75);
	CHECK(m.active());
	CHECK_NEAR(m.render().amount, 1.0, 1e-9);
	CHECK_NEAR(m.render().zoom, 2.0, 1e-9);
}

TEST(magnifier_canvas_stops_at_the_edge_but_view_reaches_the_corner)
{
	MagnifierConfig cfg;
	cfg.enabled = true;
	cfg.follow_cursor = true;
	cfg.radius = 0.28;
	cfg.in_ms = 0.0;

	MagnifierModel m;
	m.configure(cfg);

	/* Landscape target: the x radius is smaller than the y radius. */
	const double aspect = 16.0 / 9.0;
	const double rx = 0.28 * (9.0 / 16.0);
	const double ry = 0.28;

	m.update(0.0, 0.0, 0.0, aspect);
	MagnifierRender r = m.render();
	CHECK_NEAR(r.sample_x, 0.0, 1e-9);
	CHECK_NEAR(r.sample_y, 0.0, 1e-9);
	CHECK_NEAR(r.focus_x, rx, 1e-9);
	CHECK_NEAR(r.focus_y, ry, 1e-9);

	m.update(1.0, 1.0, 1.0, aspect);
	r = m.render();
	CHECK_NEAR(r.sample_x, 1.0, 1e-9);
	CHECK_NEAR(r.sample_y, 1.0, 1e-9);
	CHECK_NEAR(r.focus_x, 1.0 - rx, 1e-9);
	CHECK_NEAR(r.focus_y, 1.0 - ry, 1e-9);

	/* Center: the canvas follows exactly. */
	m.update(2.0, 0.5, 0.5, aspect);
	r = m.render();
	CHECK_NEAR(r.focus_x, 0.5, 1e-9);
	CHECK_NEAR(r.focus_y, 0.5, 1e-9);
	CHECK_NEAR(r.sample_x, 0.5, 1e-9);
	CHECK_NEAR(r.sample_y, 0.5, 1e-9);

	/* The canvas never leaves the screen. */
	m.update(3.0, 0.9, 0.1, aspect);
	r = m.render();
	CHECK(r.focus_x - rx >= -1e-9);
	CHECK(r.focus_x + rx <= 1.0 + 1e-9);
	CHECK(r.focus_y - ry >= -1e-9);
	CHECK(r.focus_y + ry <= 1.0 + 1e-9);
}

TEST(magnifier_fixed_center_ignores_cursor)
{
	MagnifierConfig cfg;
	cfg.follow_cursor = false;
	cfg.center_x = 0.2;
	cfg.center_y = 0.8;
	cfg.enabled = true;

	MagnifierModel m;
	m.configure(cfg);
	m.update(0.0, 0.9, 0.1);
	CHECK_NEAR(m.render().focus_x, 0.2, 1e-9);
	CHECK_NEAR(m.render().focus_y, 0.8, 1e-9);
	/* A fixed center shows a fixed area, not the cursor. */
	CHECK_NEAR(m.render().sample_x, 0.2, 1e-9);
	CHECK_NEAR(m.render().sample_y, 0.8, 1e-9);
}

TEST(magnifier_animates_out)
{
	MagnifierConfig cfg;
	cfg.enabled = true;
	cfg.in_ms = 0.0;
	cfg.out_ms = 100.0;

	MagnifierModel m;
	m.configure(cfg);
	CHECK_NEAR(m.render().amount, 1.0, 1e-9);

	m.set_active(false, 0.0);
	m.update(50.0, 0.5, 0.5);
	const MagnifierRender mid = m.render();
	CHECK(mid.amount > 0.0 && mid.amount < 1.0);
	CHECK_NEAR(mid.zoom, 1.0 + (2.0 - 1.0) * mid.amount, 1e-9);

	m.update(100.0, 0.5, 0.5);
	CHECK(!m.active());
	CHECK_NEAR(m.render().amount, 0.0, 1e-9);
}
