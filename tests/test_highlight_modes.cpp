#include "core/geometry.hpp"
#include "core/highlighter.hpp"
#include "core/pen.hpp"
#include "core/types.hpp"
#include "core/zoom.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

namespace {
Vec2f pt(double x, double y)
{
	Vec2f p;
	p.x = static_cast<float>(x);
	p.y = static_cast<float>(y);
	return p;
}
} // namespace

/* ---- line mode ---------------------------------------------------------- */

TEST(highlight_line_records_the_trail)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Line);
	m.configure(cfg);

	m.begin(0.1, 0.2, 0.0);
	m.move(0.2, 0.3, 10.0);
	m.move(0.5, 0.6, 20.0);
	m.end(30.0);
	m.update(40.0);

	const HighlightRender r = m.render();
	REQUIRE(r.active);
	CHECK_EQ(r.mode, static_cast<int>(HighlightMode::Line));
	CHECK(r.points != nullptr);
	CHECK(r.point_count >= 3);
	CHECK_NEAR(r.points[0].x, 0.1, 1e-6);
	CHECK_NEAR(r.points[r.point_count - 1].y, 0.6, 1e-6);
}

TEST(highlight_line_ignores_duplicate_points)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Line);
	m.configure(cfg);

	/* A short (but valid) stroke, then the same point repeated 50 times. */
	m.begin(0.2, 0.2, 0.0);
	m.move(0.35, 0.35, 5.0);
	const int after_stroke = m.render().point_count;
	for (int i = 0; i < 50; ++i)
		m.move(0.35, 0.35, static_cast<double>(10 + i));
	m.end(60.0);
	m.update(70.0);

	CHECK_EQ(m.render().point_count, after_stroke);
	CHECK(after_stroke >= 2);
}

TEST(highlight_line_trail_is_capped)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Line);
	m.configure(cfg);

	m.begin(0.0, 0.0, 0.0);
	for (int i = 0; i < 5000; ++i)
		m.move(static_cast<double>(i) / 5000.0, 0.5, static_cast<double>(i));
	m.end(6000.0);
	m.update(6100.0);

	CHECK(m.render().point_count <= 512);
}

/* ---- zoom gesture ------------------------------------------------------- */

TEST(highlight_zoom_mode_draws_nothing_and_requests_a_zoom)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Zoom);
	m.configure(cfg);

	m.begin(0.2, 0.2, 0.0);
	m.move(0.6, 0.7, 100.0);
	m.end(200.0);
	m.update(250.0);

	CHECK(!m.render().active); /* nothing is painted */

	double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.2, 1e-9);
	CHECK_NEAR(y1, 0.7, 1e-9);
	/* The request is consumed once. */
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));
}

TEST(highlight_zoom_mode_ignores_clicks)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Zoom);
	m.configure(cfg);

	m.begin(0.5, 0.5, 0.0);
	m.move(0.505, 0.505, 10.0);
	m.end(20.0);

	double x0, y0, x1, y1;
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));
}

TEST(zoom_spot_goes_in_hold_and_back)
{
	ZoomModel z;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Off); /* keep the normal zoom at 1x */
	z.configure(cfg);
	z.reset(0.0);
	z.spot_start_hold(0.25, 0.75, 3.0, 200.0, 500.0, 800.0, 0.0);

	/* Half of the zoom-in: between 1x and 3x, and it stays while held. */
	z.update(100.0);
	const double mid = z.render().zoom;
	CHECK(mid > 1.0);
	CHECK(mid < 3.0);
	z.update(5000.0);
	CHECK(z.spot_active());
	CHECK_NEAR(z.render().zoom, 3.0, 1e-6);

	/* Focused on the point. */
	const ZoomRender held = z.render();
	CHECK(held.focus_x > 0.0 && held.focus_x <= 0.25 + 1e-6);

	/* Release at 5100: hold (500 ms) keeps it at the top... */
	z.spot_release(5100.0);
	z.update(5300.0);
	CHECK_NEAR(z.render().zoom, 3.0, 1e-6);

	/* ...then it glides back... */
	z.update(6000.0);
	const double back = z.render().zoom;
	CHECK(back > 1.0);
	CHECK(back < 3.0);

	/* ...and the normal zoom takes over again. */
	z.update(8000.0);
	CHECK(!z.spot_active());
	CHECK_NEAR(z.render().zoom, 1.0, 1e-6);
}

TEST(zoom_spot_aim_rect_fits_and_centres)
{
	/* A 0.2x0.2 rect from a 1x base: zoom 5x, centred on (0.3, 0.7). */
	const SpotAim a = spot_aim_for_hold(true, 0.2, 0.6, 0.4, 0.8, 0.0, 0.0, 1.0, 2.5, 8.0,
					    false);
	REQUIRE(a.valid);
	CHECK_NEAR(a.zoom, 5.0, 1e-9);
	/* The selection centre must land on the screen centre (shader law). */
	CHECK_NEAR(a.focus_x + (0.3 - a.focus_x) * a.zoom, 0.5, 1e-9);
	CHECK_NEAR(a.focus_y + (0.7 - a.focus_y) * a.zoom, 0.5, 1e-9);

	/* A huge rect is a no-op (nothing to zoom into). */
	const SpotAim b = spot_aim_for_hold(true, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 2.5, 8.0,
					    false);
	CHECK(!b.valid);
}

TEST(zoom_spot_aim_point_uses_max_and_current_cursor)
{
	/* Plain hold at 1x: zoom to the configured maximum, centred on the cursor. */
	const SpotAim a = spot_aim_for_hold(false, 0, 0, 0, 0, 0.8, 0.2, 1.0, 2.5, 8.0, false);
	REQUIRE(a.valid);
	CHECK_NEAR(a.zoom, 2.5, 1e-9);
	CHECK_NEAR(a.focus_x + (0.8 - a.focus_x) * a.zoom, 0.5, 1e-9);

	/* Already at/past the maximum: nothing to do. */
	CHECK(!spot_aim_for_hold(false, 0, 0, 0, 0, 0.5, 0.5, 2.5, 2.5, 8.0, false).valid);
	CHECK(!spot_aim_for_hold(false, 0, 0, 0, 0, 0.5, 0.5, 4.0, 2.5, 8.0, false).valid);
}

TEST(zoom_spot_aim_out_widens_and_never_goes_below_one)
{
	/* From 4x, widening by half. */
	const SpotAim a = spot_aim_for_hold(false, 0, 0, 0, 0, 0.5, 0.5, 4.0, 2.0, 8.0, true);
	REQUIRE(a.valid);
	CHECK_NEAR(a.zoom, 2.0, 1e-9);

	/* From 1x there is nowhere to go. */
	CHECK(!spot_aim_for_hold(false, 0, 0, 0, 0, 0.5, 0.5, 1.0, 2.0, 8.0, true).valid);
}

TEST(zoom_spot_starts_from_and_returns_to_the_current_zoom)
{
	ZoomModel z;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Manual);
	z.configure(cfg);
	z.reset(0.0);
	z.set_manual(2.0);
	for (int t = 50; t <= 5000; t += 50)
		z.update(static_cast<double>(t));
	const double base = z.render().zoom;
	CHECK_NEAR(base, 2.0, 0.05); /* spring settled at the manual zoom */

	/* A plain-hold aim from the current zoom: 2.5x absolute. */
	const SpotAim aim = spot_aim_for_hold(false, 0, 0, 0, 0, 0.5, 0.5, base, 2.5, 8.0, false);
	REQUIRE(aim.valid);
	CHECK_NEAR(aim.zoom, 2.5, 1e-9);

	z.spot_start_hold(aim.focus_x, aim.focus_y, aim.zoom, 100.0, 0.0, 300.0, 5100.0);
	z.update(5200.0); /* end of the in phase, still held */
	CHECK_NEAR(z.render().zoom, 2.5, 0.05);

	/* Release: back to exactly the zoom that was on screen, not 1x. */
	z.spot_release(5300.0);
	z.update(8000.0);
	CHECK(!z.spot_active());
	CHECK_NEAR(z.render().zoom, base, 0.05);
}

TEST(zoom_spot_punch_has_no_hold)
{
	ZoomModel z;
	ZoomConfig cfg;
	cfg.mode = static_cast<int>(ZoomMode::Off);
	z.configure(cfg);
	z.reset(0.0);
	z.update(10.0);

	z.spot_start_hold(0.4, 0.6, 2.0, 100.0, 0.0, 200.0, 20.0);
	z.update(120.0); /* exactly the end of the in phase */
	const double at_in = z.render().zoom;
	CHECK_NEAR(at_in, 2.0, 1e-6);

	/* Release right away: with no hold the return starts immediately. */
	z.spot_release(120.0);
	z.update(160.0);
	CHECK(z.render().zoom < at_in);
	z.update(1000.0);
	CHECK(!z.spot_active());
}

/* Only the box mode may paint the rectangle: the line has its own overlay and
 * the zoom gesture must draw nothing at all. Regression for "Line still shows
 * the filled box". */
TEST(highlight_only_box_mode_paints_the_rectangle)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Box);
	m.configure(cfg);
	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 10.0);
	m.end(20.0);
	m.update(30.0);
	CHECK(paints_box(m.render()));

	cfg.mode = static_cast<int>(HighlightMode::Line);
	m.configure(cfg);
	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 10.0);
	m.end(20.0);
	m.update(30.0);
	CHECK(!paints_box(m.render()));

	cfg.mode = static_cast<int>(HighlightMode::Zoom);
	m.configure(cfg);
	m.begin(0.1, 0.1, 0.0);
	m.move(0.5, 0.5, 10.0);
	m.end(20.0);
	m.update(30.0);
	CHECK(!paints_box(m.render()));
}

/* The centre of the selection must end up in the centre of the screen. */
TEST(zoom_spot_focus_centres_the_selection)
{
	for (double z : {1.5, 2.0, 3.0}) {
		for (double sc : {0.35, 0.5, 0.62}) {
			const double f = center_focus_for(sc, z);
			/* Where the selection centre appears on screen (shader law). */
			const double shown = f + (sc - f) * z;
			CHECK_NEAR(shown, 0.5, 1e-9);
		}
	}
}

/* The zoom request fires while the button is STILL held, after the delay. */
TEST(highlight_zoom_fires_on_hold_after_delay)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Zoom);
	cfg.zoom_delay_ms = 1200.0;
	m.configure(cfg);

	m.begin(0.5, 0.5, 0.0);

	/* Held, but not long enough: nothing yet. */
	m.update(1199.0);
	double x0, y0, x1, y1;
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));

	/* Held past the delay: the request fires while dragging. */
	m.update(1300.0);
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.5, 1e-9); /* no drag: point request at the cursor */
	CHECK_NEAR(x1, 0.5, 1e-9);

	/* Consumed once: no repeat while still held. */
	m.update(5000.0);
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));

	/* New press starts over. */
	m.end(5100.0);
	m.begin(0.2, 0.2, 5200.0);
	m.update(5300.0);
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));
	m.update(6500.0);
	CHECK(m.take_zoom_request(&x0, &y0, &x1, &y1));
}

/* Punch fires on the drag itself, without waiting for the delay. */
TEST(highlight_zoom_punch_fires_on_drag)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Zoom);
	cfg.zoom_dir = 2; /* punch in */
	cfg.zoom_delay_ms = 1200.0;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.25, 0.3, 10.0);
	m.update(11.0); /* 1000+ ms before the delay */

	double x0, y0, x1, y1;
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.1, 1e-9);
	CHECK_NEAR(x1, 0.25, 1e-9);
}

/* Punch without any movement stays quiet: clicking must not zoom. */
TEST(highlight_zoom_punch_needs_movement)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Zoom);
	cfg.zoom_dir = 3; /* punch out */
	cfg.zoom_delay_ms = 1200.0;
	m.configure(cfg);

	m.begin(0.4, 0.4, 0.0);
	m.update(100.0);
	m.end(150.0);

	double x0, y0, x1, y1;
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));
}

/* Releasing without ever holding long enough keeps the old quick behaviour
 * when a real drag happened. */
TEST(highlight_zoom_quick_drag_still_fires_on_release)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::Zoom);
	cfg.zoom_delay_ms = 1200.0;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.4, 0.5, 100.0);
	m.end(300.0); /* 300 ms < 1200 ms delay */

	double x0, y0, x1, y1;
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.1, 1e-9);
	CHECK_NEAR(y1, 0.5, 1e-9);
}
