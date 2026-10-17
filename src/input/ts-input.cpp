#include "ts-input.hpp"

namespace topscreen {

void Input::start(obs_source_t *source)
{
	stop();
	platform_ = ts_input_platform_start(source);
}

void Input::stop()
{
	if (platform_) {
		ts_input_platform_stop(platform_);
		platform_ = nullptr;
	}
}

void Input::poll()
{
	/* Only rising edges use the previous state; the key lists
	 * are transient per tick and do not need to be copied. */
	const bool was_left = prev_left_;
	const bool was_right = prev_right_;
	prev_left_ = cur_.left;
	prev_right_ = cur_.right;

	InputState s;
	if (platform_)
		ts_input_platform_poll(platform_, &s);

	s.left_pressed = s.left && !was_left;
	s.right_pressed = s.right && !was_right;

	cur_ = s;
}

} // namespace topscreen
