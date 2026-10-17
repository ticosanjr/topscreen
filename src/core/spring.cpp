#include "spring.hpp"

#include <cmath>

namespace topscreen::core {

void Spring::reset(double value)
{
	value_ = value;
	target_ = value;
	velocity_ = 0.0;
}

void Spring::set_stiffness(double s)
{
	stiffness_ = s > 0.0 ? s : 0.0;
}

void Spring::set_damping(double d)
{
	damping_ = d > 0.0 ? d : 1.0;
}

void Spring::set_target(double target)
{
	target_ = target;
}

double Spring::update(double dt)
{
	/* Clamps dt for stability during frame drops. */
	if (dt > 0.05)
		dt = 0.05;
	if (dt <= 0.0)
		return value_;

	const double k = stiffness_;
	const double c = 2.0 * damping_ * std::sqrt(k);

	/*
	 * Semi-implicit integration is stable only if h <= ~0.25/sqrt(k).
	 * For high stiffness we subdivide the step (substeps), avoiding
	 * divergence when the frame rate drops.
	 */
	double max_h = (k > 0.0) ? (0.25 / std::sqrt(k)) : dt;
	if (max_h <= 0.0 || max_h > dt)
		max_h = dt;

	int steps = static_cast<int>(std::ceil(dt / max_h));
	if (steps < 1)
		steps = 1;
	if (steps > 64)
		steps = 64;
	const double h = dt / steps;

	for (int i = 0; i < steps; ++i) {
		const double a = -k * (value_ - target_) - c * velocity_;
		velocity_ += a * h;
		value_ += velocity_ * h;
	}
	return value_;
}

} // namespace topscreen::core
