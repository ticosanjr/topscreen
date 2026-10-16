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
	prev_ = cur_;

	InputState s;
	if (platform_)
		ts_input_platform_poll(platform_, &s);

	s.left_pressed = s.left && !prev_.left;
	s.right_pressed = s.right && !prev_.right;

	cur_ = s;
}

} // namespace topscreen
