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

TEST(zoom_manual_step_and_limits)
{
	ZoomModel m;
	ZoomConfig cfg;
	m.configure(cfg);
	m.reset(0.0);

	CHECK_NEAR(m.manual(), 1.0, 1e-9);
	m.manual_step(1.25);
	CHECK_NEAR(m.manual(), 1.25, 1e-9);
	m.manual_step(0.8);
	CHECK_NEAR(m.manual(), 1.0, 1e-9); /* never below 1 */
	for (int i = 0; i < 40; ++i)
		m.manual_step(2.0);
	CHECK(m.manual() <= 8.0 + 1e-9);
}

TEST(zoom_auto_zooms_with_recent_focus)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 2.0;
	cfg.duration_ms = 200.0;
	cfg.idle_timeout_ms = 100000.0; /* does not expire during the test */
	cfg.pan_amount = 1.0;
	m.configure(cfg);
	m.reset(0.0);

	m.focus_on(0.5, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 1.0);
	CHECK(m.render().zoom > 1.5);
}

TEST(zoom_auto_returns_to_normal_on_idle)
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

	/* Passes the idle_timeout: must return to 1.0. */
	advance(m, t, 2.0);
	CHECK_NEAR(m.render().zoom, 1.0, 0.02);
}

TEST(zoom_off_mode_does_not_zoom)
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

TEST(zoom_centers_the_cursor)
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
	CHECK_NEAR(c, 0.5, 0.02); /* cursor at the center */
}

TEST(zoom_anticipates_corner)
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

	/* Cursor at 20% from the left: the view must already be at the edge. */
	m.focus_on(0.2, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 1.5);

	const ZoomRender r = m.render();
	CHECK(r.focus_x < 0.05);

	/* At the leftmost pixel, the sampled source is the corner (0). */
	const double u_no_pixel_0 = r.focus_x + (0.0 - r.focus_x) / r.zoom;
	CHECK(u_no_pixel_0 <= 0.02);
}
TEST(zoom_focus_reaches_corners)
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

	/* Bottom-right corner of the source. */
	m.focus_on(1.0, 1.0, 0.0);
	double t = 0.0;
	advance(m, t, 1.0);
	const ZoomRender r = m.render();
	CHECK(r.focus_x > 0.9);
	CHECK(r.focus_y > 0.9);

	/* The source point (1,1) must appear INSIDE the screen. */
	const double d = r.focus_x + (1.0 - r.focus_x) * r.zoom;
	CHECK(d <= 1.0 + 1e-6);
}

TEST(zoom_left_corner_with_small_pan)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 1.6;
	cfg.duration_ms = 100.0;
	cfg.idle_timeout_ms = 100000.0;
	cfg.pan_amount = 0.25; /* "soft" pan: even so the corner must be reached */
	m.configure(cfg);
	m.reset(0.0);

	m.focus_on(0.0, 0.0, 0.0);
	double t = 0.0;
	advance(m, t, 1.5);

	const ZoomRender r = m.render();
	/* the focus must be pulled close to 0 (not stay at 0.375) */
	CHECK(r.focus_x < 0.1);
	CHECK(r.focus_y < 0.1);

	/* and the point (0,0) must be visible (0 <= d <= 1) */
	const double d = r.focus_x + (0.0 - r.focus_x) * r.zoom;
	CHECK(d >= -1e-6);
	CHECK(d <= 1.0 + 1e-6);
}

TEST(zoom_pan_does_not_decay_over_time)
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

	/* much later, the focus must not have "decayed" toward the center */
	advance(m, t, 3.0);
	const double fx2 = m.render().focus_x;
	CHECK_NEAR(fx1, fx2, 1e-6);
	CHECK(fx2 > 0.6);
}

TEST(zoom_smart_zooms_on_activity_and_returns_after_hold)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Smart);
	cfg.amount = 2.0;
	cfg.duration_ms = 100.0;
	cfg.smart_hold_ms = 300.0;
	m.configure(cfg);
	m.reset(0.0);
	m.note_activity(0.0);

	double t = 0.0;
	advance(m, t, 0.2);
	CHECK(m.render().zoom > 1.4);

	advance(m, t, 2.0);
	CHECK(m.render().zoom < 1.1);
}

TEST(zoom_smart_region_focuses_the_active_area)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Smart);
	cfg.amount = 3.0;
	cfg.duration_ms = 100.0;
	cfg.smart_hold_ms = 1000.0;
	m.configure(cfg);
	m.reset(0.0);
	m.set_region(0.2, 0.2, 0.8, 0.8);
	m.note_activity(0.0);

	double t = 0.0;
	advance(m, t, 0.5);
	const ZoomRender r = m.render();
	CHECK(r.zoom > 1.2);
	CHECK(r.zoom < 3.0);
	CHECK_NEAR(r.focus_x, 0.5, 0.05);
	CHECK_NEAR(r.focus_y, 0.5, 0.05);
}

TEST(zoom_locked_spot_stays_until_release)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Off);
	cfg.duration_ms = 50.0;
	m.configure(cfg);
	m.reset(0.0);

	m.spot_start_locked(0.7, 0.3, 2.0, 50.0, 1200.0, 0.0);
	m.update(1000.0);
	const ZoomRender held = m.render();
	CHECK_NEAR(held.zoom, 2.0, 1e-6);
	CHECK_NEAR(held.focus_x, 0.7, 1e-6);
	CHECK_NEAR(held.focus_y, 0.3, 1e-6);

	/* Still locked after a long time. */
	m.update(100000.0);
	CHECK_NEAR(m.render().zoom, 2.0, 1e-6);
	CHECK_NEAR(m.render().focus_x, 0.7, 1e-6);

	m.spot_release(100001.0);
	m.update(100002.0);
	CHECK(m.spot_active());
	m.update(200000.0);
	CHECK(!m.spot_active());
	CHECK_NEAR(m.render().zoom, 1.0, 1e-6);
}

TEST(zoom_freeze_holds_the_current_view)
{
	ZoomModel m;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Auto);
	cfg.amount = 2.0;
	cfg.duration_ms = 50.0;
	m.configure(cfg);
	m.reset(0.0);
	m.set_cursor(0.2, 0.3, 0.0);
	m.update(200.0);
	const double z = m.render().zoom;

	m.freeze();
	m.set_cursor(0.9, 0.9, 300.0);
	m.update(400.0);
	CHECK_NEAR(m.render().zoom, z, 1e-6);
	m.unfreeze();
}
