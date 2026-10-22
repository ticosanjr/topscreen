#pragma once

#include <obs-module.h>

#include <cstdint>
#include <string>

namespace topscreen {

/** Input state (cursor + buttons + keys) captured per tick. */
struct InputState {
	static constexpr int kMaxKeyEvents = 8;

	double x = 0.0;
	double y = 0.0;
	bool left = false;
	bool right = false;
	bool left_pressed = false;  /* rising edge this tick */
	bool right_pressed = false; /* rising edge this tick */
	bool link = false;          /* native pointer is over a link (Win/X11) */
	uint32_t modifiers = 0;     /* core::KeyModifier bitmask */
	bool available = false;

	/* Keys with a rising edge this tick (for the overlay). */
	std::string key_labels[kMaxKeyEvents];
	bool key_modifier[kMaxKeyEvents] = {};
	int key_count = 0;

	/* Capability tier: 0 = unavailable, 1 = native, 2 = portal, 3 = evdev. */
	int tier = 0;
};

/** Platform-independent input manager. */
class Input {
public:
	void start(obs_source_t *source);
	void stop();
	void poll();

	const InputState &state() const { return cur_; }
	bool available() const { return cur_.available; }
	int tier() const { return cur_.tier; }

private:
	InputState cur_;
	/* Only the rising edges are compared; without copying key lists. */
	bool prev_left_ = false;
	bool prev_right_ = false;
	void *platform_ = nullptr;
};

/* ---- Backend interface (implemented in ts-input-platform.cpp) ---- */
void *ts_input_platform_start(obs_source_t *source);
void ts_input_platform_stop(void *state);
void ts_input_platform_poll(void *state, InputState *out);

} // namespace topscreen
