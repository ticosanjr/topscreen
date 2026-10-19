#include "core/coord.hpp"
#include "core/monitor.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

namespace {

Monitor mon(double x, double y, double w, double h)
{
	Monitor m;
	m.x = x;
	m.y = y;
	m.width = w;
	m.height = h;
	return m;
}

} // namespace

/* A capture of the monitor BELOW the primary one: global y starts at 1080. */
TEST(coord_global_point_below_primary_monitor)
{
	const Monitor lower = mon(0.0, 1080.0, 1920.0, 1080.0);

	/* Middle of the lower screen in global coordinates. */
	const Vec2f c = screen_to_content(960.0, 1620.0, lower);
	CHECK_NEAR(c.x, 0.5, 1e-9);
	CHECK_NEAR(c.y, 0.5, 1e-9);

	/* Top-left and bottom-right corners of that monitor. */
	const Vec2f tl = screen_to_content(0.0, 1080.0, lower);
	CHECK_NEAR(tl.x, 0.0, 1e-9);
	CHECK_NEAR(tl.y, 0.0, 1e-9);

	const Vec2f br = screen_to_content(1920.0, 2160.0, lower);
	CHECK_NEAR(br.x, 1.0, 1e-9);
	CHECK_NEAR(br.y, 1.0, 1e-9);
}

/* Monitor to the RIGHT with a different size (offset + width). */
TEST(coord_global_point_right_monitor)
{
	const Monitor right = mon(1920.0, 0.0, 3440.0, 1440.0);

	const Vec2f c = screen_to_content(1920.0 + 1720.0, 720.0, right);
	CHECK_NEAR(c.x, 0.5, 1e-9);
	CHECK_NEAR(c.y, 0.5, 1e-9);
}

/* Monitor to the LEFT of the primary one: negative global x. */
TEST(coord_global_point_left_monitor)
{
	const Monitor left = mon(-1920.0, 0.0, 1920.0, 1080.0);

	const Vec2f c = screen_to_content(-960.0, 540.0, left);
	CHECK_NEAR(c.x, 0.5, 1e-9);
	CHECK_NEAR(c.y, 0.5, 1e-9);
}

/* Monitor ABOVE the primary one: negative global y. */
TEST(coord_global_point_above_monitor)
{
	const Monitor above = mon(0.0, -1080.0, 1920.0, 1080.0);

	const Vec2f c = screen_to_content(960.0, -540.0, above);
	CHECK_NEAR(c.x, 0.5, 1e-9);
	CHECK_NEAR(c.y, 0.5, 1e-9);
}

/* Without the origin (0,0) the old, broken mapping is reproduced: this is the
 * regression the fix is about (y would be 1.5 instead of 0.5). */
TEST(coord_missing_origin_misplaces_below_monitor)
{
	const Monitor wrong = mon(0.0, 0.0, 1920.0, 1080.0);
	const Vec2f c = screen_to_content(960.0, 1620.0, wrong);
	CHECK_NEAR(c.y, 1.5, 1e-9); /* out of range: cursor pinned to the border */
}

/* Round trip: content -> screen -> content. */
TEST(coord_round_trip_with_offset)
{
	const Monitor below = mon(0.0, 1080.0, 2560.0, 1440.0);
	for (double u : {0.0, 0.25, 0.5, 0.75, 1.0}) {
		for (double v : {0.0, 0.3, 0.5, 0.9, 1.0}) {
			const Vec2f s = content_to_screen(u, v, below);
			const Vec2f c = screen_to_content(s.x, s.y, below);
			CHECK_NEAR(c.x, u, 1e-9);
			CHECK_NEAR(c.y, v, 1e-9);
		}
	}
}

/* hint + matching size wins. */
TEST(monitor_pick_hint_with_matching_size)
{
	const Monitor mons[2] = {mon(0, 0, 1920, 1080), mon(0, 1080, 1920, 1080)};
	Monitor out;
	REQUIRE(pick_monitor(mons, 2, 1, true, 1920, 1080, &out));
	CHECK_NEAR(out.y, 1080.0, 1e-9);
}

/* Unique size match is used even without a trustworthy hint (X11 case):
 * the capture is the 3440x1440 screen, so its origin is (1920, -4). */
TEST(monitor_pick_unique_size_match)
{
	const Monitor mons[2] = {mon(0, 0, 1920, 1080), mon(1920, -4, 3440, 1440)};
	Monitor out;
	REQUIRE(pick_monitor(mons, 2, 0, false, 3440, 1440, &out));
	CHECK_NEAR(out.x, 1920.0, 1e-9);
	CHECK_NEAR(out.y, -4.0, 1e-9);
}

/* Hint is trusted only when there is no unique size match. */
TEST(monitor_pick_trusted_hint_wins_over_ambiguity)
{
	const Monitor mons[2] = {mon(0, 0, 1920, 1080), mon(0, 1080, 1920, 1080)};
	Monitor out;

	/* Two identical monitors + trusted hint. */
	REQUIRE(pick_monitor(mons, 2, 1, true, 1920, 1080, &out));
	CHECK_NEAR(out.y, 1080.0, 1e-9);

	/* Same ambiguity without a trustworthy hint: undecidable. */
	CHECK(!pick_monitor(mons, 2, 0, false, 1600, 900, &out));
}

/* Degenerate inputs never crash and never pick. */
TEST(monitor_pick_invalid_inputs)
{
	Monitor out;
	CHECK(!pick_monitor(nullptr, 4, 0, true, 1920, 1080, &out));
	CHECK(!pick_monitor(nullptr, 0, 0, false, 1920, 1080, &out));

	const Monitor mons[1] = {mon(0, 0, 1920, 1080)};
	CHECK(!pick_monitor(mons, 1, 0, true, 0, 0, &out)); /* size 0 never matches */
	CHECK(!pick_monitor(mons, 1, 5, false, 640, 480, &out)); /* no hint, no match */
	CHECK(!pick_monitor(mons, 1, 0, false, 640, 480, &out));
}
