#include "core/geometry.hpp"
#include "core/highlighter.hpp"
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

/* ---- line decimation (the shader declares a fixed number of slots) ------- */

TEST(subsample_line_keeps_every_point_when_it_fits)
{
	Vec2f in[3] = {pt(0.1, 0.2), pt(0.3, 0.4), pt(0.5, 0.6)};
	Vec2f out[16];
	CHECK_EQ(subsample_line(in, 3, out, 16), 3);
	CHECK_NEAR(out[0].x, 0.1, 1e-6);
	CHECK_NEAR(out[2].y, 0.6, 1e-6);
}

TEST(subsample_line_keeps_first_and_last_and_caps_at_max)
{
	Vec2f in[100];
	for (int i = 0; i < 100; ++i)
		in[i] = pt(static_cast<double>(i) / 99.0, 0.25);

	Vec2f out[16];
	const int n = subsample_line(in, 100, out, 16);
	CHECK_EQ(n, 16);
	CHECK_NEAR(out[0].x, 0.0, 1e-6);
	CHECK_NEAR(out[15].x, 1.0, 1e-6);
	/* Monotonic and strictly spread: every sampled index moves forward. */
	for (int i = 1; i < n; ++i)
		CHECK(out[i].x > out[i - 1].x);
	/* Sampling is even: index step stays inside one bin. */
	CHECK(out[1].x <= 0.5);
	CHECK(out[14].x >= 0.5);
}

/* Arc-length sampling keeps the segments even when the pointer moved at
 * different speeds (dense points in slow parts, sparse in fast parts). */
TEST(subsample_line_resamples_by_distance)
{
	/* First half has 60 dense points, second half only 3 points, all on a
	 * straight line from x=0 to x=1. Equal-index sampling would crowd the
	 * output in the dense half. */
	Vec2f in[64];
	int n = 0;
	for (int i = 0; i < 60; ++i)
		in[n++] = pt(0.0 + 0.50 * static_cast<double>(i) / 59.0, 0.0);
	in[n++] = pt(0.70, 0.0);
	in[n++] = pt(0.85, 0.0);
	in[n++] = pt(1.00, 0.0);

	Vec2f out[16];
	const int got = subsample_line(in, n, out, 16);
	CHECK_EQ(got, 16);
	for (int i = 1; i < got; ++i) {
		const double step = out[i].x - out[i - 1].x;
		CHECK_NEAR(step, 1.0 / 15.0, 0.02);
	}
}

TEST(subsample_line_handles_degenerate_input)
{
	Vec2f in[1] = {pt(0.7, 0.8)};
	Vec2f out[16];
	CHECK_EQ(subsample_line(nullptr, 4, out, 16), 0);
	CHECK_EQ(subsample_line(in, 0, out, 16), 0);
	CHECK_EQ(subsample_line(in, 1, out, 0), 0);
	CHECK_EQ(subsample_line(in, 1, out, 1), 1);
	CHECK_NEAR(out[0].x, 0.7, 1e-6);
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

/* Only the box modes may paint the rectangle: the line is drawn by the main
 * shader and the zoom gesture must draw nothing at all. Regression for "Line
 * still shows the filled box". */
TEST(highlight_only_box_modes_paint_the_rectangle)
{
	for (int mode : {static_cast<int>(HighlightMode::Box),
			 static_cast<int>(HighlightMode::BoxZoom)}) {
		HighlighterModel m;
		HighlighterConfig cfg;
		cfg.mode = mode;
		m.configure(cfg);
		m.begin(0.1, 0.1, 0.0);
		m.move(0.5, 0.5, 10.0);
		m.end(20.0);
		m.update(30.0);
		CHECK(paints_box(m.render()));
	}
	for (int mode : {static_cast<int>(HighlightMode::Line),
			 static_cast<int>(HighlightMode::LineZoom),
			 static_cast<int>(HighlightMode::Zoom)}) {
		HighlighterModel m;
		HighlighterConfig cfg;
		cfg.mode = mode;
		m.configure(cfg);
		m.begin(0.1, 0.1, 0.0);
		if (mode_draws_line(mode)) {
			m.move(0.3, 0.3, 10.0);
			m.move(0.5, 0.5, 20.0);
		} else {
			m.move(0.5, 0.5, 10.0);
		}
		m.end(20.0);
		m.update(30.0);
		if (mode_draws_line(mode))
			CHECK(m.render().active);
		CHECK(!paints_box(m.render()));
	}
}

/* The two predicates are what the compositor and the properties UI rely on:
 * pin them for every mode so a future mode cannot silently draw the wrong
 * thing (that class of bug already cost us "line shows a box" and "line
 * draws nothing"). */
TEST(highlight_mode_predicates_are_pinned)
{
	struct Row {
		HighlightMode mode;
		bool box;
		bool line;
		bool zoom;
	};
	const Row rows[] = {
		{HighlightMode::Box, true, false, false},
		{HighlightMode::Line, false, true, false},
		{HighlightMode::Zoom, false, false, true},
		{HighlightMode::BoxZoom, true, false, true},
		{HighlightMode::LineZoom, false, true, true},
	};

	for (const Row &row : rows) {
		const int mode = static_cast<int>(row.mode);
		HighlighterModel m;
		HighlighterConfig cfg;
		cfg.mode = mode;
		cfg.enabled = true;
		m.configure(cfg);

		m.begin(0.1, 0.2, 0.0);
		m.move(0.3, 0.4, 10.0);
		m.move(0.6, 0.7, 20.0);
		m.end(30.0);
		for (double t = 31.0; t < 400.0; t += 37.0)
			m.update(t);

		const HighlightRender r = m.render();
		/* The model must agree with the predicates used by the renderer. */
		CHECK_EQ(paints_box(r), row.box);
		CHECK_EQ(draws_line(r), row.line);
		CHECK_EQ(mode_wants_zoom(mode), row.zoom);
		CHECK_EQ(mode_draws_line(mode), row.line);
		CHECK_EQ(mode_draws_mark(mode), row.box || row.line);
		if (row.box) {
			CHECK(r.active);
		} else if (row.line) {
			/* The line mark is live and carries its trail. */
			CHECK(r.active);
			CHECK(r.points != nullptr);
			CHECK(r.point_count >= 3);
		} else {
			CHECK(!r.active);
		}
	}
}

/* LineZoom draws the freehand line AND asks for a zoom (same hold-to-zoom
 * semantics as BoxZoom, but the mark is the stroke instead of the box). */
TEST(highlight_linezoom_draws_the_line_and_zooms)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::LineZoom);
	cfg.zoom_delay_ms = 100.0;
	m.configure(cfg);

	/* Hold past the delay while drawing: the request fires with the line. */
	m.begin(0.1, 0.1, 0.0);
	m.update(50.0);
	double x0, y0, x1, y1;
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));

	m.move(0.25, 0.2, 60.0);
	m.move(0.4, 0.5, 80.0);
	m.update(200.0);
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	/* The selection is the stroke's bounding box (stored as floats). */
	CHECK_NEAR(x0, 0.1, 1e-6);
	CHECK_NEAR(y0, 0.1, 1e-6);
	CHECK_NEAR(x1, 0.4, 1e-6);
	CHECK_NEAR(y1, 0.5, 1e-6);

	/* ...and it is only requested once. */
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));

	const HighlightRender r = m.render();
	CHECK(r.active);
	CHECK(draws_line(r));
	CHECK(!paints_box(r));
	REQUIRE(r.points != nullptr);
	CHECK(r.point_count >= 3);
	CHECK_NEAR(r.points[0].x, 0.1, 1e-6);

	/* The line stays on screen (and fades) after the release, exactly like
	 * BoxZoom keeps its box - unlike the pure zoom mode. */
	m.end(300.0);
	m.update(400.0);
	CHECK(m.render().active);
	CHECK(draws_line(m.render()));
}

/* Released before the delay: the zoom still runs (with the drawn line). */
TEST(highlight_linezoom_quick_drag_fires_on_release)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::LineZoom);
	cfg.zoom_delay_ms = 1200.0;
	m.configure(cfg);

	m.begin(0.1, 0.1, 0.0);
	m.move(0.2, 0.4, 40.0);
	m.move(0.5, 0.6, 80.0);
	m.end(300.0); /* 300 ms < 1200 ms delay */

	double x0, y0, x1, y1;
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.1, 1e-6);
	CHECK_NEAR(x1, 0.5, 1e-6);
	CHECK(draws_line(m.render()));
}

/* Holding without drawing: the zoom still fires (point zoom) but there is no
 * stroke to show, so nothing is drawn. */
TEST(highlight_linezoom_hold_without_drawing_zooms_without_a_line)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::LineZoom);
	cfg.zoom_delay_ms = 100.0;
	m.configure(cfg);

	m.begin(0.5, 0.5, 0.0);
	m.update(300.0);

	double x0, y0, x1, y1;
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.5, 1e-9);
	CHECK_NEAR(x1, 0.5, 1e-9);
	CHECK(!m.render().active); /* fewer than two trail points: nothing to draw */
}

/* A freehand circle ends where it started, so the start/end rectangle is
 * degenerate. The zoom must use the stroke's bounding box instead: circling a
 * region has to zoom into that region (and not into a single point). */
TEST(highlight_linezoom_circle_zooms_into_the_drawn_area)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::LineZoom);
	cfg.zoom_delay_ms = 50.0;
	m.configure(cfg);

	m.begin(0.40, 0.40, 0.0);
	m.move(0.60, 0.40, 10.0);
	m.move(0.60, 0.60, 20.0);
	m.move(0.40, 0.60, 30.0);
	m.move(0.40, 0.40, 40.0); /* back at the start: closed shape */
	m.update(200.0);

	double x0, y0, x1, y1;
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.40, 1e-6);
	CHECK_NEAR(y0, 0.40, 1e-6);
	CHECK_NEAR(x1, 0.60, 1e-6);
	CHECK_NEAR(y1, 0.60, 1e-6);
	CHECK(draws_line(m.render())); /* the circle is still shown while zoomed */
}

/* The box modes zoom into exactly the rectangle that is on screen, so the
 * selection follows the start/end drag (not the widest extent ever reached). */
TEST(highlight_boxzoom_selection_matches_the_box_on_screen)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::BoxZoom);
	cfg.zoom_delay_ms = 50.0;
	m.configure(cfg);

	m.begin(0.2, 0.3, 0.0);
	m.move(0.9, 0.8, 10.0);
	m.move(0.5, 0.5, 20.0); /* the drag ends here: this is the box on screen */
	m.update(200.0);

	double x0, y0, x1, y1;
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.2, 1e-9);
	CHECK_NEAR(y0, 0.3, 1e-9);
	CHECK_NEAR(x1, 0.5, 1e-9);
	CHECK_NEAR(y1, 0.5, 1e-9);
	CHECK_NEAR(m.render().x1, 0.5, 1e-9);
}

/* BoxZoom draws its box AND asks for a zoom (hold-to-zoom semantics). */
TEST(highlight_boxzoom_draws_and_zooms)
{
	HighlighterModel m;
	HighlighterConfig cfg;
	cfg.mode = static_cast<int>(HighlightMode::BoxZoom);
	cfg.zoom_delay_ms = 100.0;
	m.configure(cfg);

	/* Hold past the delay: the zoom request fires with the drawn box. */
	m.begin(0.1, 0.1, 0.0);
	m.update(50.0);
	double x0, y0, x1, y1;
	CHECK(!m.take_zoom_request(&x0, &y0, &x1, &y1));
	m.move(0.4, 0.5, 60.0);
	m.update(200.0);
	REQUIRE(m.take_zoom_request(&x0, &y0, &x1, &y1));
	CHECK_NEAR(x0, 0.1, 1e-9);
	CHECK_NEAR(y1, 0.5, 1e-9);

	/* ...while the box stays painted (fades after release). */
	const HighlightRender r = m.render();
	CHECK(r.active);
	CHECK(paints_box(r));
	CHECK_NEAR(r.x1, 0.4, 1e-9);

	m.end(300.0);
	m.update(400.0);
	CHECK(m.render().active); /* still fading, like the plain box */
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
