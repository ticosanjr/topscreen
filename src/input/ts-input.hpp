#pragma once

#include <obs-module.h>

#include <string>

namespace topscreen {

/** Estado de input (cursor + botoes + teclas) capturado por tick. */
struct InputState {
	static constexpr int kMaxKeyEvents = 8;

	double x = 0.0;
	double y = 0.0;
	bool left = false;
	bool right = false;
	bool left_pressed = false;  /* borda de subida neste tick */
	bool right_pressed = false; /* borda de subida neste tick */
	bool available = false;

	/* Teclas com borda de subida neste tick (para o overlay). */
	std::string key_labels[kMaxKeyEvents];
	bool key_modifier[kMaxKeyEvents] = {};
	int key_count = 0;

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
	/* So as bordas de subida sao comparadas; sem copiar listas de teclas. */
	bool prev_left_ = false;
	bool prev_right_ = false;
	void *platform_ = nullptr;
};

/* ---- Interface de backend (implementada em ts-input-platform.cpp) ---- */
void *ts_input_platform_start(obs_source_t *source);
void ts_input_platform_stop(void *state);
void ts_input_platform_poll(void *state, InputState *out);

} // namespace topscreen
