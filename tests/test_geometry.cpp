#include "core/geometry.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(geometry_sd_round_box_sign)
{
	/* Center: negative (inside). Outside: positive. */
	CHECK(sd_round_box(0.0, 0.0, 100.0, 50.0, 10.0) < 0.0);
	CHECK(sd_round_box(200.0, 0.0, 100.0, 50.0, 10.0) > 0.0);
	/* Straight edge: ~0. */
	CHECK_NEAR(sd_round_box(100.0, 0.0, 100.0, 50.0, 0.0), 0.0, 1e-9);
	/* Rounded corner: the point (100,40) is on the edge of radius 10. */
	CHECK_NEAR(sd_round_box(100.0, 40.0, 100.0, 50.0, 10.0), 0.0, 1e-9);
	/* A point at the "square" corner lies outside the rounded rectangle. */
	CHECK(sd_round_box(100.0, 50.0, 100.0, 50.0, 10.0) > 0.0);
}

TEST(geometry_mask_center_and_outside)
{
	CHECK_NEAR(rounded_rect_mask(0.0, 0.0, 100.0, 50.0, 10.0, 1.0), 1.0, 1e-6);
	CHECK_NEAR(rounded_rect_mask(500.0, 500.0, 100.0, 50.0, 10.0, 1.0), 0.0, 1e-6);
	/* At the square corner (outside the radius) it must be 0. */
	CHECK_NEAR(rounded_rect_mask(99.0, 49.0, 100.0, 50.0, 10.0, 1.0), 0.0, 1e-3);
}

TEST(geometry_zoom_uv_identity)
{
	const Vec2f uv = zoom_uv(0.3, 0.7, 0.5, 0.5, 1.0);
	CHECK_NEAR(uv.x, 0.3, 1e-9);
	CHECK_NEAR(uv.y, 0.7, 1e-9);
}

TEST(geometry_zoom_uv_zooms_around_focus)
{
	/* With focus at 0.5 and zoom 2, the edge (0) maps to 0.25. */
	const Vec2f uv = zoom_uv(0.0, 1.0, 0.5, 0.5, 2.0);
	CHECK_NEAR(uv.x, 0.25, 1e-9);
	CHECK_NEAR(uv.y, 0.75, 1e-9);
	/* The focus stays at the focus. */
	const Vec2f f = zoom_uv(0.5, 0.5, 0.5, 0.5, 3.0);
	CHECK_NEAR(f.x, 0.5, 1e-9);
}

TEST(geometry_pixel_to_content)
{
	const Vec2f c = pixel_to_content(50.0, 50.0, 1100.0, 700.0, 50.0);
	CHECK_NEAR(c.x, 0.0, 1e-9);
	CHECK_NEAR(c.y, 0.0, 1e-9);

	const Vec2f mid = pixel_to_content(550.0, 350.0, 1100.0, 700.0, 50.0);
	CHECK_NEAR(mid.x, 0.5, 1e-9);
	CHECK_NEAR(mid.y, 0.5, 1e-9);
}

TEST(geometry_center_focus)
{
	/* Source center with any zoom -> focus 0.5 (cursor at the center). */
	CHECK_NEAR(center_focus_for(0.5, 2.0), 0.5, 1e-9);

	/* Edges -> focus at the limit (0/1). */
	CHECK_NEAR(center_focus_for(0.0, 1.6), 0.0, 1e-9);
	CHECK_NEAR(center_focus_for(1.0, 1.6), 1.0, 1e-9);

	/*
	 * Anticipation: with zoom 1.6, the cursor at s=0.3125 already puts the view
	 * at the left edge (focus 0) -> the corner appears BEFORE the mouse arrives.
	 */
	CHECK_NEAR(center_focus_for(0.3125, 1.6), 0.0, 1e-6);
	CHECK(center_focus_for(0.6, 1.6) > 0.0);

	/* Without zoom the focus does not matter. */
	CHECK_NEAR(center_focus_for(0.0, 1.0), 0.5, 1e-9);
}

TEST(geometry_center_focus_keeps_cursor_visible)
{
	for (double z : {1.3, 1.6, 2.0, 3.0}) {
		for (double s = 0.0; s <= 1.0; s += 0.05) {
			const double f = center_focus_for(s, z);
			const double c = f + (s - f) * z;
			CHECK(c >= -1e-9 && c <= 1.0 + 1e-9);
		}
	}
}
TEST(geometry_clamp_focus_corners)
{
	/* Focus in [0,1]: the corners are reachable (the 1/zoom window fits in the source). */
	CHECK_NEAR(clamp_focus(0.0, 2.0), 0.0, 1e-9);
	CHECK_NEAR(clamp_focus(1.0, 2.0), 1.0, 1e-9);
	CHECK_NEAR(clamp_focus(-0.5, 2.0), 0.0, 1e-9);
	CHECK_NEAR(clamp_focus(1.5, 2.0), 1.0, 1e-9);
	CHECK_NEAR(clamp_focus(0.5, 1.0), 0.5, 1e-9);
}

TEST(geometry_focus_limits_keep_point_visible)
{
	/* Point at the left corner: the focus must go to 0 for it to appear. */
	FocusLimits l = focus_limits(0.0, 2.0);
	CHECK_NEAR(l.lo, 0.0, 1e-9);
	CHECK_NEAR(l.hi, 0.0, 1e-9);
	CHECK_NEAR(clamp_focus_keep(0.0, 0.8, 2.0), 0.0, 1e-9);

	/* Point at the right corner: focus 1. */
	l = focus_limits(1.0, 2.0);
	CHECK_NEAR(l.lo, 1.0, 1e-9);
	CHECK_NEAR(l.hi, 1.0, 1e-9);

	/* At the center the range allows any focus (free pan). */
	l = focus_limits(0.5, 2.0);
	CHECK(l.lo <= 1e-9);
	CHECK(l.hi >= 1.0 - 1e-9);

	/* Without zoom: any focus works. */
	l = focus_limits(0.0, 1.0);
	CHECK_NEAR(l.lo, 0.0, 1e-9);
	CHECK_NEAR(l.hi, 1.0, 1e-9);
}
