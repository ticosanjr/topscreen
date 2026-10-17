#pragma once

namespace topscreen::core {

/**
 * Critically damped 1D spring.
 *
 * Used to smooth the cursor, zoom and pan without "jumps": the value chases the target
 * with acceleration proportional to the displacement and damping proportional to the
 * velocity. damping = 1.0 => critically damped (no overshoot).
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
	double target() const { return target_; }

private:
	double value_ = 0.0;
	double target_ = 0.0;
	double velocity_ = 0.0;
	double stiffness_ = 120.0;
	double damping_ = 1.0;
};

} // namespace topscreen::core
