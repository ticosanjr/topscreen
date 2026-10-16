#pragma once

#include <obs-module.h>

namespace topscreen {

/** Estado de input (cursor + botoes) capturado por tick. */
struct InputState {
	double x = 0.0;
	double y = 0.0;
	bool left = false;
	bool right = false;
	bool left_pressed = false;  /* borda de subida neste tick */
	bool right_pressed = false; /* borda de subida neste tick */
	bool available = false;

	/* Tier de capacidade: 0 = indisponivel, 1 = nativo, 2 = portal, 3 = evdev. */
	int tier = 0;
};

/** Gerenciador de input independente de plataforma. */
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
	InputState prev_;
	void *platform_ = nullptr;
};

/* ---- Interface de backend (implementada em ts-input-platform.cpp) ---- */
void *ts_input_platform_start(obs_source_t *source);
void ts_input_platform_stop(void *state);
void ts_input_platform_poll(void *state, InputState *out);

} // namespace topscreen
