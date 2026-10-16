#pragma once

namespace topscreen {

/**
 * Mola 1D criticamente amortecida.
 *
 * Usada para suavizar cursor, zoom e pan sem "saltos": o valor persegue o alvo
 * com aceleracao proporcional ao deslocamento e amortecimento proporcional a
 * velocidade. damping = 1.0 => criticamente amortecida (sem overshoot).
 */
class Spring {
public:
	void reset(double value);
	void set_stiffness(double s);
	void set_damping(double d);
	void set_target(double target);
	double update(double dt);

	double value() const { return value_; }
	double velocity() const { return velocity_; }

private:
	double value_ = 0.0;
	double target_ = 0.0;
	double velocity_ = 0.0;
	double stiffness_ = 120.0;
	double damping_ = 1.0;
};

/* Easings normalizados (entrada 0..1, saida 0..1). */
double ease_linear(double t);
double ease_in_out_cubic(double t);
double ease_out_cubic(double t);
double ease_in_cubic(double t);

} // namespace topscreen
