#pragma once

#include <cstdint>

namespace topscreen::core {

/** Bit flags for the modifier keys held while a key is pressed. */
enum KeyModifier : uint32_t {
	KeyModShift = 1u << 0,
	KeyModCtrl = 1u << 1,
	KeyModAlt = 1u << 2,
	KeyModWin = 1u << 3,
};

inline const char *key_modifier_name(KeyModifier mod)
{
	switch (mod) {
	case KeyModShift:
		return "Shift";
	case KeyModCtrl:
		return "Ctrl";
	case KeyModAlt:
		return "Alt";
	case KeyModWin:
		return "Win";
	}
	return "";
}

} // namespace topscreen::core
