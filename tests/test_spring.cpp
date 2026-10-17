#include "core/spring.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

namespace {
double run_to(Spring &s, double target, double seconds, double dt = 1.0 / 60.0)
{
	s.set_target(target);
	double max_val = -1e9;
	double min_val = 1e9;
	for (double t = 0.0; t < seconds; t += dt) {
		const double v = s.update(dt);
		if (v > max_val)
			max_val = v;
		if (v < min_val)
			min_val = v;
	}
	return (target >= 0.0) ? max_val : min_val;
}
} // namespace

TEST(spring_converges_to_target)
{
	Spring s;
	s.set_stiffness(120.0);
	s.set_damping(1.0);
	s.reset(0.0);
	run_to(s, 1.0, 2.0);
	CHECK_NEAR(s.value(), 1.0, 1e-3);
}

TEST(spring_critical_no_relevant_overshoot)
{
	Spring s;
	s.set_stiffness(120.0);
	s.set_damping(1.0);
	s.reset(0.0);
	const double max_val = run_to(s, 1.0, 1.5);
	/* Critically damped: overshoot must be minimal. */
	CHECK(max_val <= 1.0 + 0.02);
}

TEST(spring_reset_zeroes_velocity)
{
	Spring s;
	s.reset(5.0);
	CHECK_NEAR(s.value(), 5.0, 1e-9);
	CHECK_NEAR(s.velocity(), 0.0, 1e-9);
	CHECK_NEAR(s.target(), 5.0, 1e-9);
}

TEST(spring_large_dt_does_not_explode)
{
	Spring s;
	s.set_stiffness(500.0);
	s.reset(0.0);
	s.set_target(1.0);
	for (int i = 0; i < 10; ++i)
		s.update(1.0); /* absurdly large dt */
	CHECK(s.value() < 10.0);
	CHECK(s.value() > -10.0);
}
