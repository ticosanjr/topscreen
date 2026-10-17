#pragma once

namespace topscreen::core {

/** Easing functions (input 0..1, output 0..1). */
enum Easing {
	EASE_LINEAR = 0,
	EASE_IN_CUBIC = 1,
	EASE_OUT_CUBIC = 2,
	EASE_IN_OUT_CUBIC = 3,
	EASE_COUNT,
};

double ease_linear(double t);
double ease_in_cubic(double t);
double ease_out_cubic(double t);
double ease_in_out_cubic(double t);

/** Applies the easing by id, clamping t to 0..1. */
double ease_apply(int easing, double t);

} // namespace topscreen::core
