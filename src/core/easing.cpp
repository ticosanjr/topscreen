#include "easing.hpp"

namespace topscreen::core {

double ease_linear(double t)
{
	return t;
}

double ease_in_cubic(double t)
{
	return t * t * t;
}

double ease_out_cubic(double t)
{
	const double u = 1.0 - t;
	return 1.0 - u * u * u;
}

double ease_in_out_cubic(double t)
{
	if (t < 0.5)
		return 4.0 * t * t * t;
	const double u = -2.0 * t + 2.0;
	return 1.0 - (u * u * u) / 2.0;
}

double ease_apply(int easing, double t)
{
	if (t <= 0.0)
		return 0.0;
	if (t >= 1.0)
		return 1.0;

	switch (easing) {
	case EASE_LINEAR:
		return ease_linear(t);
	case EASE_IN_CUBIC:
		return ease_in_cubic(t);
	case EASE_OUT_CUBIC:
		return ease_out_cubic(t);
	case EASE_IN_OUT_CUBIC:
	default:
		return ease_in_out_cubic(t);
	}
}

} // namespace topscreen::core
