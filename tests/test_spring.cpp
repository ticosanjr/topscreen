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

TEST(spring_converge_para_alvo)
{
	Spring s;
	s.set_stiffness(120.0);
	s.set_damping(1.0);
	s.reset(0.0);
	run_to(s, 1.0, 2.0);
	CHECK_NEAR(s.value(), 1.0, 1e-3);
}

TEST(spring_critico_sem_overshoot_relevante)
{
	Spring s;
	s.set_stiffness(120.0);
	s.set_damping(1.0);
	s.reset(0.0);
	const double max_val = run_to(s, 1.0, 1.5);
	/* Criticamente amortecida: overshoot deve ser minimo. */
	CHECK(max_val <= 1.0 + 0.02);
}

TEST(spring_reset_zera_velocidade)
{
	Spring s;
	s.reset(5.0);
	CHECK_NEAR(s.value(), 5.0, 1e-9);
	CHECK_NEAR(s.velocity(), 0.0, 1e-9);
	CHECK_NEAR(s.target(), 5.0, 1e-9);
}

TEST(spring_dt_grande_nao_explode)
{
	Spring s;
	s.set_stiffness(500.0);
	s.reset(0.0);
	s.set_target(1.0);
	for (int i = 0; i < 10; ++i)
		s.update(1.0); /* dt absurdamente grande */
	CHECK(s.value() < 10.0);
	CHECK(s.value() > -10.0);
}
