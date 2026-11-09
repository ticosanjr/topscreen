#include "ts-input.hpp"

/*
 * Platform input backend: Windows WinAPI (cursor, buttons and key scan).
 * Empty on every other platform.
 */

#if defined(_WIN32)

#include <windows.h>

#include <string>

#include "core/key_modifiers.hpp"

namespace topscreen {

namespace {

struct WinInput {
	unsigned char prev[256] = {};
};

std::string win_unicode_label(int vk)
{
	BYTE state[256] = {};
	if (!GetKeyboardState(state))
		return {};

	wchar_t text[8] = {};
	const int n = ToUnicodeEx(vk, MapVirtualKey(vk, MAPVK_VK_TO_VSC), state, text,
				  static_cast<int>(sizeof(text) / sizeof(text[0])), 0,
				  GetKeyboardLayout(0));
	if (n <= 0)
		return {};
	if (n == 1 && text[0] < 0x20)
		return {};

	const int bytes = WideCharToMultiByte(CP_UTF8, 0, text, n, nullptr, 0, nullptr, nullptr);
	if (bytes <= 0)
		return {};
	std::string out(static_cast<size_t>(bytes), '\0');
	WideCharToMultiByte(CP_UTF8, 0, text, n, out.data(), bytes, nullptr, nullptr);
	return out;
}

std::string win_key_label(int vk, bool *is_mod, bool shift)
{
	*is_mod = false;

	if (vk >= 'A' && vk <= 'Z')
		return std::string(1, static_cast<char>(vk));
	if (vk >= '0' && vk <= '9') {
		if (shift) {
			const std::string text = win_unicode_label(vk);
			if (!text.empty())
				return text;
		}
		return std::string(1, static_cast<char>(vk));
	}

	switch (vk) {
	case VK_SPACE:
		return "Space";
	case VK_RETURN:
		return "Enter";
	case VK_TAB:
		return "Tab";
	case VK_ESCAPE:
		return "Esc";
	case VK_BACK:
		return "Backspace";
	case VK_DELETE:
		return "Del";
	case VK_LEFT:
		return "Left";
	case VK_RIGHT:
		return "Right";
	case VK_UP:
		return "Up";
	case VK_DOWN:
		return "Down";
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
		*is_mod = true;
		return "Shift";
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
		*is_mod = true;
		return "Ctrl";
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
		*is_mod = true;
		return "Alt";
	case VK_LWIN:
	case VK_RWIN:
		*is_mod = true;
		return "Win";
	default:
		switch (vk) {
		case VK_OEM_1:
		case VK_OEM_PLUS:
		case VK_OEM_COMMA:
		case VK_OEM_MINUS:
		case VK_OEM_PERIOD:
		case VK_OEM_2:
		case VK_OEM_3:
		case VK_OEM_4:
		case VK_OEM_5:
		case VK_OEM_6:
		case VK_OEM_7:
		case VK_OEM_8:
		case VK_OEM_102:
			return win_unicode_label(vk);
		default:
			return {};
		}
	}
}

} // namespace

void *ts_input_platform_start(obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	return new WinInput();
}

void ts_input_platform_stop(void *state)
{
	delete static_cast<WinInput *>(state);
}

void ts_input_platform_poll(void *state, InputState *out)
{
	auto *s = static_cast<WinInput *>(state);

	POINT pt;
	if (GetCursorPos(&pt)) {
		out->x = static_cast<double>(pt.x);
		out->y = static_cast<double>(pt.y);
		out->available = true;
		out->tier = 1;
	}

	out->left = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	out->right = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

	/* Native cursor role: the standard link hand is IDC_HAND. Custom cursor
	 * themes still map their link cursor to this system handle. */
	CURSORINFO ci{};
	ci.cbSize = sizeof(ci);
	if (GetCursorInfo(&ci)) {
		static const HCURSOR hand = LoadCursor(nullptr, IDC_HAND);
		out->link = (ci.hCursor == hand);
	}

	BYTE keyboard[256] = {};
	GetKeyboardState(keyboard);
	const bool shift = (keyboard[VK_SHIFT] & 0x80) != 0 || (keyboard[VK_LSHIFT] & 0x80) != 0 ||
			   (keyboard[VK_RSHIFT] & 0x80) != 0;
	if ((keyboard[VK_CONTROL] & 0x80) || (keyboard[VK_LCONTROL] & 0x80) ||
	    (keyboard[VK_RCONTROL] & 0x80))
		out->modifiers |= core::KeyModCtrl;
	if (shift)
		out->modifiers |= core::KeyModShift;
	if ((keyboard[VK_MENU] & 0x80) || (keyboard[VK_LMENU] & 0x80) ||
	    (keyboard[VK_RMENU] & 0x80))
		out->modifiers |= core::KeyModAlt;
	if ((keyboard[VK_LWIN] & 0x80) || (keyboard[VK_RWIN] & 0x80))
		out->modifiers |= core::KeyModWin;

	/* Only the keys win_key_label() understands (buttons handled above). */
	static const int kWatched[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		VK_SPACE, VK_RETURN, VK_TAB, VK_ESCAPE, VK_BACK, VK_DELETE,
		VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN,
		VK_SHIFT, VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN,
		VK_OEM_1, VK_OEM_PLUS, VK_OEM_COMMA, VK_OEM_MINUS, VK_OEM_PERIOD,
		VK_OEM_2, VK_OEM_3, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_OEM_7,
		VK_OEM_8, VK_OEM_102,
	};
	for (size_t i = 0; i < sizeof(kWatched) / sizeof(kWatched[0]); ++i) {
		const int vk = kWatched[i];
		const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
		if (down && !s->prev[vk]) {
			bool is_mod = false;
			const std::string label = win_key_label(vk, &is_mod, shift);
			if (!label.empty() && out->key_count < InputState::kMaxKeyEvents) {
				out->key_labels[out->key_count] = label;
				out->key_modifier[out->key_count] = is_mod;
				out->key_modifiers[out->key_count] = out->modifiers;
				++out->key_count;
			}
		}
		s->prev[vk] = down ? 1 : 0;
	}
}

} // namespace topscreen

#endif
