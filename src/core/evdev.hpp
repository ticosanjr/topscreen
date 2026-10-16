#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Evento de tecla decodificado de /dev/input (Tier 3 - evdev). */
struct EvdevKey {
	int code = 0;
	bool pressed = false;
};

/** Tamanho de struct input_event em 64-bit (timeval 16 + u16 + u16 + s32). */
inline constexpr size_t kEvdevRecord64 = 24;

/** Rotulo legivel para um keycode do Linux ("" se desconhecido). */
std::string evdev_key_label(int code);

/** true se o keycode e um modificador (Ctrl/Shift/Alt/Meta). */
bool evdev_key_is_modifier(int code);

/**
 * Decodifica registros de input_event (EV_KEY).
 *
 * `record_size` e o tamanho de struct input_event no host (varia por ABI).
 * Retorna true se ao menos um registro completo foi lido.
 */
bool evdev_decode(const uint8_t *data, size_t size, size_t record_size, std::vector<EvdevKey> *out);

} // namespace topscreen::core
