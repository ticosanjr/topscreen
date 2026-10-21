#include "core/spotlight.hpp"

#include "test_framework.hpp"

using namespace topscreen::core;

namespace {

SpotlightConfig cfg()
{
	SpotlightConfig c;
	c.enabled = false;
	c.zoom_amount = 2.0;
	c.in_ms = 100.0;
	c.out_ms = 100.0;
	c.drop_px = 20.0;
	return c;
}

} // namespace

TEST(spotlight_starts_inactive)
{
	SpotlightModel m;
	SpotlightConfig c = cfg();
	m.configure(c);
	m.update(0.0, 0.5, 0.5);

	const SpotlightRender r = m.render();
	CHECK_NEAR(r.amount, 0.0, 1e-9);
	CHECK_NEAR(r.zoom, 1.0, 1e-9);
	CHECK_NEAR(r.drop, 0.0, 1e-9);
	CHECK(!m.active());
}

TEST(spotlight_animates_in_with_drop_and_zoom)
{
	SpotlightModel m;
	SpotlightConfig c = cfg();
	m.configure(c);
	m.set_active(true, 0.0);
	m.update(0.0, 0.2, 0.3);

	SpotlightRender r = m.render();
	CHECK_NEAR(r.amount, 0.0, 1e-9);
	CHECK_NEAR(r.drop, -20.0, 1e-9);
	CHECK_NEAR(r.focus_x, 0.2, 1e-9);
	CHECK_NEAR(r.focus_y, 0.3, 1e-9);

	m.update(50.0, 0.2, 0.3);
	r = m.render();
	CHECK(r.amount > 0.0 && r.amount < 1.0);
	CHECK(r.drop < 0.0 && r.drop > -20.0);
	CHECK(r.zoom > 1.0 && r.zoom < 2.0);

	m.update(100.0, 0.2, 0.3);
	r = m.render();
	CHECK_NEAR(r.amount, 1.0, 1e-9);
	CHECK_NEAR(r.drop, 0.0, 1e-9);
	CHECK_NEAR(r.zoom, 2.0, 1e-9);
	CHECK(m.active());
}

TEST(spotlight_animates_out_with_drop_and_zoom)
{
	SpotlightModel m;
	SpotlightConfig c = cfg();
	c.enabled = true;
	m.configure(c);
	m.update(0.0, 0.5, 0.5);
	m.set_active(false, 100.0);
	m.update(150.0, 0.5, 0.5);

	SpotlightRender r = m.render();
	CHECK(r.amount > 0.0 && r.amount < 1.0);
	CHECK(r.drop > 0.0 && r.drop < 20.0);
	CHECK(r.zoom > 1.0 && r.zoom < 2.0);

	m.update(200.0, 0.5, 0.5);
	r = m.render();
	CHECK_NEAR(r.amount, 0.0, 1e-9);
	CHECK_NEAR(r.zoom, 1.0, 1e-9);
	CHECK(!m.active());
}

TEST(spotlight_reverses_mid_transition)
{
	SpotlightModel m;
	SpotlightConfig c = cfg();
	m.configure(c);
	m.set_active(true, 0.0);
	m.update(40.0, 0.5, 0.5);
	const double peak = m.render().amount;
	CHECK(peak > 0.0 && peak < 1.0);

	m.set_active(false, 40.0);
	m.update(90.0, 0.5, 0.5);
	CHECK(m.render().amount < peak);
	m.update(140.0, 0.5, 0.5);
	CHECK_NEAR(m.render().amount, 0.0, 1e-9);
	CHECK(!m.active());
}

TEST(spotlight_initial_enabled_is_fully_active)
{
	SpotlightModel m;
	SpotlightConfig c = cfg();
	c.enabled = true;
	m.configure(c);
	m.update(10.0, 0.25, 0.75);

	const SpotlightRender r = m.render();
	CHECK_NEAR(r.amount, 1.0, 1e-9);
	CHECK_NEAR(r.zoom, 2.0, 1e-9);
	CHECK_NEAR(r.focus_x, 0.25, 1e-9);
	CHECK_NEAR(r.focus_y, 0.75, 1e-9);
	CHECK(m.active());
}
