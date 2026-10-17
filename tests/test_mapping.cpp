#include "core/geometry.hpp"
#include "core/mapping.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(mapping_area_without_padding)
{
	const ContentArea a = content_area(1920, 1080, 0, 0, false);
	CHECK_NEAR(a.origin.x, 0.0, 1e-9);
	CHECK_NEAR(a.origin.y, 0.0, 1e-9);
	CHECK_NEAR(a.size.x, 1920.0, 1e-9);
	CHECK_NEAR(a.size.y, 1080.0, 1e-9);
}

TEST(mapping_area_with_padding)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, false);
	CHECK_NEAR(a.origin.x, 64.0, 1e-9);
	CHECK_NEAR(a.origin.y, 64.0, 1e-9);
	CHECK_NEAR(a.size.x, 1792.0, 1e-9);
	CHECK_NEAR(a.size.y, 952.0, 1e-9);
}

TEST(mapping_area_keep_aspect)
{
	/* 1920x1080 with padding 64/64: sc = min(1792/1920, 952/1080) = 0.88148... */
	const ContentArea a = content_area(1920, 1080, 64, 64, true);
	const double sc = 952.0 / 1080.0;
	CHECK_NEAR(a.size.x, 1920.0 * sc, 1e-6);
	CHECK_NEAR(a.size.y, 1080.0 * sc, 1e-6);
	CHECK_NEAR(a.origin.x, (1920.0 - 1920.0 * sc) * 0.5, 1e-6);
	CHECK_NEAR(a.origin.y, (1080.0 - 1080.0 * sc) * 0.5, 1e-6);
}

TEST(mapping_zoom_identity)
{
	const Vec2f d = source_to_display(0.3, 0.7, 0.5, 0.5, 1.0);
	CHECK_NEAR(d.x, 0.3, 1e-9);
	CHECK_NEAR(d.y, 0.7, 1e-9);
}

TEST(mapping_zoom_forward)
{
	/* With focus 0.5 and zoom 2, a point 0.1 away from the focus appears 0.2 away. */
	const Vec2f d = source_to_display(0.6, 0.4, 0.5, 0.5, 2.0);
	CHECK_NEAR(d.x, 0.7, 1e-9);
	CHECK_NEAR(d.y, 0.3, 1e-9);

	/* And it is the inverse of the shader sampling: u = f + (d - f)/z. */
	const double u = 0.5 + (d.x - 0.5) / 2.0;
	CHECK_NEAR(u, 0.6, 1e-9);
}

TEST(mapping_source_to_pixel)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, false);
	/* source center -> content center */
	const Vec2f c = source_to_pixel(0.5, 0.5, 0.5, 0.5, 1.0, a);
	CHECK_NEAR(c.x, 64.0 + 0.5 * 1792.0, 1e-6);
	CHECK_NEAR(c.y, 64.0 + 0.5 * 952.0, 1e-6);

	/* top-left corner of the source -> content corner */
	const Vec2f tl = source_to_pixel(0.0, 0.0, 0.5, 0.5, 1.0, a);
	CHECK_NEAR(tl.x, 64.0, 1e-6);
	CHECK_NEAR(tl.y, 64.0, 1e-6);
}

/*
 * The corners: with the focus adjusted to keep the point visible, a point on
 * the EDGE has to appear inside the content area (it does not vanish off screen).
 */
TEST(mapping_corner_visible_with_adjusted_focus)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, true);
	const double pontos[] = {0.0, 0.25, 0.5, 0.75, 1.0};
	const double zooms[] = {1.6, 2.5};

	for (double s : pontos) {
		for (double z : zooms) {
			/* worst case: centered focus (pan 0) -> the clamp has to pull */
			const double f = clamp_focus_keep(s, 0.5, z);
			const Vec2f d = source_to_display(s, s, f, f, z);
			CHECK(d.x >= -1e-9 && d.x <= 1.0 + 1e-9);
			CHECK(d.y >= -1e-9 && d.y <= 1.0 + 1e-9);

			const Vec2f p = source_to_pixel(s, s, f, f, z, a);
			CHECK(p.x >= a.origin.x - 1e-6);
			CHECK(p.x <= a.origin.x + a.size.x + 1e-6);
			CHECK(p.y >= a.origin.y - 1e-6);
			CHECK(p.y <= a.origin.y + a.size.y + 1e-6);
		}
	}
}
TEST(mapping_anticipates_corner)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, false);
	const double z = 1.6;
	const double s = 0.2;

	/* With centering, the focus for s=0.2 already goes to the edge (0). */
	const double f = center_focus_for(s, z);
	CHECK_NEAR(f, 0.0, 1e-9);

	/* The left corner (source 0) is already at the frame edge (anticipated). */
	const double u_no_pixel_0 = f + (0.0 - f) / z;
	CHECK(u_no_pixel_0 <= 1e-9);

	/* And the cursor remains visible. */
	const double c = f + (s - f) * z;
	CHECK(c >= -1e-9);
	CHECK(c <= 1.0 + 1e-9);

	const Vec2f p = source_to_pixel(s, s, f, f, z, a);
	CHECK(p.x >= a.origin.x - 1e-6);
	CHECK(p.x <= a.origin.x + a.size.x + 1e-6);
}

TEST(mapping_consistency_with_shader)
{
	const ContentArea a = content_area(1920, 1080, 64, 64, true);
	const double f = 0.42;
	const double z = 1.8;

	for (int i = 1; i < 10; ++i) {
		for (int j = 1; j < 10; ++j) {
			const double cx = i / 10.0;
			const double cy = j / 10.0;

			/* what the shader samples for this pixel */
			const double ux = f + (cx - f) / z;
			const double uy = f + (cy - f) / z;

			/* the overlay at that source point must land on the SAME pixel */
			const Vec2f p = source_to_pixel(ux, uy, f, f, z, a);
			CHECK_NEAR(p.x, a.origin.x + cx * a.size.x, 1e-6);
			CHECK_NEAR(p.y, a.origin.y + cy * a.size.y, 1e-6);
		}
	}
}
