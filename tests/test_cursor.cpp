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

TEST(cursor_initial_position)
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

TEST(cursor_follows_target)
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

TEST(cursor_idle_reduces_opacity)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.hide_idle = true;
	cfg.idle_delay_ms = 1000.0;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);

	double t = 0.0;
	advance(m, t, 0.5); /* before the delay */
	CHECK(m.render().opacity > 0.99);

	advance(m, t, 2.0); /* well past delay + fade */
	CHECK(m.render().opacity < 0.01);
	CHECK(!m.visible());
}

TEST(cursor_without_hide_idle_keeps_opacity)
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

TEST(cursor_loop_returns_to_start)
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
	CHECK(m.render().x > 0.5); /* already moved far away */

	/* Stays still: after loop_ms it must return close to the start. */
	advance(m, t, 4.0);
	CHECK_NEAR(m.render().x, 0.1, 0.05);
	CHECK_NEAR(m.render().y, 0.1, 0.05);
}

TEST(cursor_disabled_invisible)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.enabled = false;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);
	CHECK(!m.render().visible);
	CHECK_NEAR(m.render().opacity, 0.0, 1e-9);
}
