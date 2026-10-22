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
	CHECK_NEAR(half.focus_x, 0.25, 1e-9);
	CHECK_NEAR(half.focus_y, 0.75, 1e-9);

	m.update(100.0, 0.25, 0.75);
	CHECK(m.active());
	CHECK_NEAR(m.render().amount, 1.0, 1e-9);
	CHECK_NEAR(m.render().zoom, 2.0, 1e-9);
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
}

TEST(magnifier_animates_out)
{
	MagnifierConfig cfg;
	cfg.enabled = true;
	cfg.in_ms = 0.0;
	cfg.out_ms = 100.0;
	cfg.drop_px = 20.0;

	MagnifierModel m;
	m.configure(cfg);
	CHECK_NEAR(m.render().amount, 1.0, 1e-9);

	m.set_active(false, 0.0);
	m.update(50.0, 0.5, 0.5);
	const MagnifierRender mid = m.render();
	CHECK(mid.amount > 0.0 && mid.amount < 1.0);
	CHECK(mid.drop > 0.0);
	CHECK_NEAR(mid.zoom, 1.0 + (2.0 - 1.0) * mid.amount, 1e-9);

	m.update(100.0, 0.5, 0.5);
	CHECK(!m.active());
	CHECK_NEAR(m.render().amount, 0.0, 1e-9);
	CHECK_NEAR(m.render().drop, 0.0, 1e-9);
}
