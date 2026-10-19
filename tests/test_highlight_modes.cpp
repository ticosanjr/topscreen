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
	z.spot_start(0.25, 0.75, 3.0, 200.0, 500.0, 800.0, 0.0);

	/* Half of the zoom-in: between 1x and 3x. */
	z.update(100.0);
	const double mid = z.render().zoom;
	CHECK(mid > 1.0);
	CHECK(mid < 3.0);

	/* Hold: exactly the target, focused on the point. */
	z.update(400.0);
	const ZoomRender held = z.render();
	CHECK_NEAR(held.zoom, 3.0, 1e-6);
	CHECK(held.focus_x > 0.0 && held.focus_x <= 0.25 + 1e-6);

	/* Coming back. */
	z.update(1100.0);
	const double back = z.render().zoom;
	CHECK(back > 1.0);
	CHECK(back < 3.0);

	/* Finished: normal zoom again. */
	z.update(2000.0);
	CHECK(!z.spot_active());
	CHECK_NEAR(z.render().zoom, 1.0, 1e-6);
}

TEST(zoom_spot_stop_restores_normal_rendering)
{
	ZoomModel z;
	ZoomConfig cfg;
	z.configure(cfg);
	z.reset(0.0);
	z.spot_start(0.5, 0.5, 2.0, 100.0, 100.0, 100.0, 0.0);
	z.update(50.0);
	CHECK(z.spot_active());
	z.spot_stop();
	CHECK(!z.spot_active());
}

/* ---- pen mask ----------------------------------------------------------- */

TEST(pen_mask_draws_along_the_path)
{
	PenCanvas c;
	c.reset(16.0 / 9.0, 256, 0.01);
	CHECK(c.width() == 256);
	CHECK(c.height() == 144);

	const uint32_t before = c.revision();
	c.add_point(pt(0.1, 0.5));
	c.add_point(pt(0.9, 0.5));
	CHECK(c.revision() > before);

	const auto &a = c.alpha();
	auto idx = [&](int x, int y) { return static_cast<size_t>(y) * c.width() + x; };

	/* Painted on the line... */
	CHECK(a[idx(128, 72)] > 0);
	CHECK(a[idx(40, 72)] > 0);
	/* ...and empty far away. */
	CHECK_EQ(a[idx(128, 10)], 0);
	CHECK_EQ(a[idx(5, 5)], 0);
}

TEST(pen_mask_reset_clears)
{
	PenCanvas c;
	c.reset(1.0, 64, 0.02);
	c.add_point(pt(0.5, 0.5));
	CHECK(c.alpha()[static_cast<size_t>(32) * c.width() + 32] > 0);

	c.reset(1.0, 64, 0.02);
	for (uint8_t v : c.alpha())
		CHECK_EQ(v, 0);
}

TEST(pen_mask_thickness_scales)
{
	PenCanvas thin, thick;
	thin.reset(1.0, 128, 0.005);
	thick.reset(1.0, 128, 0.03);

	auto painted = [](const PenCanvas &c) {
		int n = 0;
		for (uint8_t v : c.alpha())
			if (v > 0)
				++n;
		return n;
	};
	thin.add_point(pt(0.5, 0.5));
	thick.add_point(pt(0.5, 0.5));
	CHECK(painted(thick) > painted(thin));
}

TEST(pen_mask_set_points_copies_a_trail)
{
	PenCanvas c;
	c.reset(1.0, 128, 0.01);
	const Vec2f pts[3] = {pt(0.2, 0.2), pt(0.5, 0.5), pt(0.8, 0.8)};
	c.set_points(pts, 3);
	CHECK(c.alpha()[static_cast<size_t>(64) * c.width() + 64] > 0);
}
