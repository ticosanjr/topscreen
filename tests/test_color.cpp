#include "core/color.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(color_unpack_known)
{
	/* 0xAABBGGRR => low byte = R. */
	const Rgba c = unpack_abgr(0xFF3A2E5C);
	CHECK_NEAR(c.r, static_cast<double>(0x5C) / 255.0, 1e-6);
	CHECK_NEAR(c.g, static_cast<double>(0x2E) / 255.0, 1e-6);
	CHECK_NEAR(c.b, static_cast<double>(0x3A) / 255.0, 1e-6);
	CHECK_NEAR(c.a, 1.0, 1e-6);
}

TEST(color_pack_roundtrip)
{
	const uint32_t values[] = {0xFFFFFFFF, 0x00000000, 0xFF3A2E5C, 0x80403020};
	for (uint32_t v : values) {
		++tst::checks();
		if (pack_abgr(unpack_abgr(v)) != v)
			tst::fail(__FILE__, __LINE__, "color roundtrip failed");
	}
}

TEST(color_lerp){
	const Rgba a{0.0f, 0.0f, 0.0f, 1.0f};
	const Rgba b{1.0f, 1.0f, 1.0f, 1.0f};

	CHECK_NEAR(lerp_color(a, b, 0.5f).r, 0.5, 1e-6);
	CHECK_NEAR(lerp_color(a, b, -1.0f).r, 0.0, 1e-6); /* clamps */
	CHECK_NEAR(lerp_color(a, b, 2.0f).r, 1.0, 1e-6);  /* clamps */
	CHECK_NEAR(lerp_color(a, b, 0.25f).g, 0.25, 1e-6);
}

/*
 * OBS stores colors as 0xAABBGGRR (obs_data/properties) but
 * gs_effect_set_color expects 0xAARRGGBB. Without the swap, red becomes blue.
 */
TEST(color_abgr_to_argb)
{
	/* red: ABGR 0xFF0000FF -> ARGB 0xFFFF0000 */
	CHECK_EQ(abgr_to_argb(0xFF0000FFu), 0xFFFF0000u);
	/* blue: ABGR 0xFFFF0000 -> ARGB 0xFF0000FF */
	CHECK_EQ(abgr_to_argb(0xFFFF0000u), 0xFF0000FFu);
	/* green passes through unchanged (middle channel) */
	CHECK_EQ(abgr_to_argb(0xFF00FF00u), 0xFF00FF00u);
	/* white/black are invariant (would not catch the bug) */
	CHECK_EQ(abgr_to_argb(0xFFFFFFFFu), 0xFFFFFFFFu);
	CHECK_EQ(abgr_to_argb(0xFF000000u), 0xFF000000u);
	/* double conversion returns to the original */
	CHECK_EQ(abgr_to_argb(abgr_to_argb(0xFF3A2E5Cu)), 0xFF3A2E5Cu);
}
