#include "core/easing.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(easing_extremes)
{
	for (int e = 0; e < EASE_COUNT; ++e) {
		CHECK_NEAR(ease_apply(e, 0.0), 0.0, 1e-9);
		CHECK_NEAR(ease_apply(e, 1.0), 1.0, 1e-9);
	}
}

TEST(easing_domain_clamp)
{
	CHECK_NEAR(ease_apply(EASE_IN_OUT_CUBIC, -5.0), 0.0, 1e-9);
	CHECK_NEAR(ease_apply(EASE_IN_OUT_CUBIC, 5.0), 1.0, 1e-9);
}

TEST(easing_monotonic)
{
	for (int e = 0; e < EASE_COUNT; ++e) {
		double prev = -1.0;
		for (int i = 0; i <= 100; ++i) {
			const double t = i / 100.0;
			const double v = ease_apply(e, t);
			CHECK(v >= prev - 1e-9);
			prev = v;
		}
	}
}

TEST(easing_in_out_symmetric)
{
	/* ease_in_out must be 0.5 in the middle. */
	CHECK_NEAR(ease_in_out_cubic(0.5), 0.5, 1e-9);
	CHECK_NEAR(ease_apply(EASE_IN_OUT_CUBIC, 0.5), 0.5, 1e-9);
}
