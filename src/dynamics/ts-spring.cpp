#include "ts-spring.hpp"

#include <cmath>

namespace topscreen {

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
	/* Limita dt para estabilidade em quedas de frame. */
	if (dt > 0.05)
		dt = 0.05;
	if (dt <= 0.0)
		return value_;

	const double k = stiffness_;
	const double c = 2.0 * damping_ * std::sqrt(k);
	const double a = -k * (value_ - target_) - c * velocity_;

	velocity_ += a * dt;
	value_ += velocity_ * dt;
	return value_;
}

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

} // namespace topscreen
