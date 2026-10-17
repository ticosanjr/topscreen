#include "evdev.hpp"

namespace topscreen::core {

namespace {
/* Linux EV_KEY codes (stable; see linux/input-event-codes.h). */
constexpr int EV_KEY_TYPE = 1;
} // namespace

std::string evdev_key_label(int code)
{
	switch (code) {
	/* Letters (QWERTY keycodes) */
	case 30: return "A"; case 48: return "B"; case 46: return "C"; case 32: return "D";
	case 18: return "E"; case 33: return "F"; case 34: return "G"; case 35: return "H";
	case 23: return "I"; case 36: return "J"; case 37: return "K"; case 38: return "L";
	case 50: return "M"; case 49: return "N"; case 24: return "O"; case 25: return "P";
	case 16: return "Q"; case 19: return "R"; case 31: return "S"; case 20: return "T";
	case 22: return "U"; case 47: return "V"; case 17: return "W"; case 45: return "X";
	case 21: return "Y"; case 44: return "Z";
	/* Digits */
	case 2: return "1"; case 3: return "2"; case 4: return "3"; case 5: return "4";
	case 6: return "5"; case 7: return "6"; case 8: return "7"; case 9: return "8";
	case 10: return "9"; case 11: return "0";
	/* Named */
	case 57: return "Space";
	case 28: return "Enter";
	case 15: return "Tab";
	case 1: return "Esc";
	case 14: return "Backspace";
	case 111: return "Del";
	case 105: return "Left";
	case 106: return "Right";
	case 103: return "Up";
	case 108: return "Down";
	/* Modifiers */
	case 29: case 97: return "Ctrl";
	case 42: case 54: return "Shift";
	case 56: case 100: return "Alt";
	case 125: case 126: return "Win";
	default: return {};
	}
}

bool evdev_key_is_modifier(int code)
{
	switch (code) {
	case 29: case 97:   /* Ctrl */
	case 42: case 54:   /* Shift */
	case 56: case 100:  /* Alt */
	case 125: case 126: /* Meta */
		return true;
	default:
		return false;
	}
}

bool evdev_decode(const uint8_t *data, size_t size, size_t record_size, std::vector<EvdevKey> *out)
{
	if (!data || !out || record_size < 8)
		return false;

	out->clear();

	const size_t count = size / record_size;
	const size_t off = record_size - 8; /* type/code/value are at the end */

	for (size_t i = 0; i < count; ++i) {
		const uint8_t *r = data + i * record_size;

		const uint16_t type = static_cast<uint16_t>(r[off] | (r[off + 1] << 8));
		const uint16_t code = static_cast<uint16_t>(r[off + 2] | (r[off + 3] << 8));
		const uint32_t value = static_cast<uint32_t>(r[off + 4]) | (static_cast<uint32_t>(r[off + 5]) << 8) |
				       (static_cast<uint32_t>(r[off + 6]) << 16) |
				       (static_cast<uint32_t>(r[off + 7]) << 24);

		if (type != EV_KEY_TYPE)
			continue;

		if (value == 0)
			out->push_back({static_cast<int>(code), false});
		else if (value == 1 || value == 2) /* press or repeat */
			out->push_back({static_cast<int>(code), true});
	}

	return count > 0;
}

} // namespace topscreen::core
