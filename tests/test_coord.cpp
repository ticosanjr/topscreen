#include "core/coord.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(coord_corners)
{
	const Monitor m{100.0, 50.0, 1920.0, 1080.0};

	const Vec2f a = screen_to_content(100.0, 50.0, m);
	CHECK_NEAR(a.x, 0.0, 1e-9);
	CHECK_NEAR(a.y, 0.0, 1e-9);

	const Vec2f b = screen_to_content(100.0 + 960.0, 50.0 + 540.0, m);
	CHECK_NEAR(b.x, 0.5, 1e-9);
	CHECK_NEAR(b.y, 0.5, 1e-9);

	const Vec2f c = screen_to_content(100.0 + 1920.0, 50.0 + 1080.0, m);
	CHECK_NEAR(c.x, 1.0, 1e-9);
	CHECK_NEAR(c.y, 1.0, 1e-9);
}

TEST(coord_roundtrip)
{
	const Monitor m{-1280.0, 0.0, 1280.0, 1024.0};
	const Vec2f c = screen_to_content(-1280.0 + 320.0, 512.0, m);
	const Vec2f s = content_to_screen(c.x, c.y, m);
	CHECK_NEAR(s.x, -960.0, 1e-9);
	CHECK_NEAR(s.y, 512.0, 1e-9);
}

TEST(coord_contains)
{
	const Monitor m{0.0, 0.0, 1920.0, 1080.0};
	CHECK(monitor_contains(m, 0.0, 0.0));
	CHECK(monitor_contains(m, 1919.0, 1079.0));
	CHECK(!monitor_contains(m, 1920.0, 500.0));
	CHECK(!monitor_contains(m, -1.0, 500.0));
}

TEST(coord_invalid_monitor_safe)
{
	const Monitor m{0.0, 0.0, 0.0, 0.0};
	const Vec2f c = screen_to_content(10.0, 10.0, m);
	CHECK_NEAR(c.x, 0.0, 1e-9);
	CHECK_NEAR(c.y, 0.0, 1e-9);
}

/* The cursor target spans the whole monitor. It must NOT be clamped to the
 * padded content rectangle: that made the drawn cursor stop short of the
 * physical pointer at the screen edges. */
TEST(coord_cursor_target_spans_whole_monitor)
{
	const Monitor m{100.0, 50.0, 1920.0, 1080.0};

	const Vec2f tl = screen_to_cursor(100.0, 50.0, m);
	CHECK_NEAR(tl.x, 0.0, 1e-9);
	CHECK_NEAR(tl.y, 0.0, 1e-9);

	const Vec2f br = screen_to_cursor(100.0 + 1920.0, 50.0 + 1080.0, m);
	CHECK_NEAR(br.x, 1.0, 1e-9);
	CHECK_NEAR(br.y, 1.0, 1e-9);

	/* Outside the monitor: clamped to the target edge, not to a content inset. */
	const Vec2f outside = screen_to_cursor(100.0 + 2400.0, 50.0 - 300.0, m);
	CHECK_NEAR(outside.x, 1.0, 1e-9);
	CHECK_NEAR(outside.y, 0.0, 1e-9);
}
