#pragma once

namespace topscreen::core {

inline double clamp01(double v)
{
	return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
}

inline double clamp_range(double v, double lo, double hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

} // namespace topscreen::core
