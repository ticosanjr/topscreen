#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Key event decoded from /dev/input (Tier 3 - evdev). */
struct EvdevKey {
	int code = 0;
	bool pressed = false;
};

/** Size of struct input_event on 64-bit (timeval 16 + u16 + u16 + s32). */
inline constexpr size_t kEvdevRecord64 = 24;

/** Readable label for a Linux keycode ("" if unknown). */
std::string evdev_key_label(int code);

/** true if the keycode is a modifier (Ctrl/Shift/Alt/Meta). */
bool evdev_key_is_modifier(int code);

/**
 * Decodes input_event records (EV_KEY).
 *
 * `record_size` is the size of struct input_event on the host (varies by ABI).
 * Returns true if at least one complete record was read.
 */
bool evdev_decode(const uint8_t *data, size_t size, size_t record_size, std::vector<EvdevKey> *out);

} // namespace topscreen::core
