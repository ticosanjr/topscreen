#include "ts-input.hpp"

/*
 * Platform input backends.
 *
 *   - Windows: WinAPI (cursor + buttons + key scan)  -> Tier 1
 *   - Linux:   X11 for cursor/buttons (Tier 1) and evdev for keys (Tier 3)
 *   - No backend: graceful degradation (Tier 4)
 *
 * Tier 2 (GlobalShortcuts portal) is planned for F7; see docs/INPUT-AND-PLATFORMS.md.
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
		VK_SHIFT, VK_LSHIFT, VK_RSHIFT, VK_CONTROL, VK_LCONTROL, VK_RCONTROL,
		VK_MENU, VK_LMENU, VK_RMENU, VK_LWIN, VK_RWIN,
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
				++out->key_count;
			}
		}
		s->prev[vk] = down ? 1 : 0;
	}
}

} // namespace topscreen

#elif defined(__linux__)

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "core/cursor.hpp"
#include "core/evdev.hpp"
#include "core/key_modifiers.hpp"

#if defined(TOPSCREEN_HAVE_X11)
#include <X11/XKBlib.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <cctype>
#if defined(TOPSCREEN_HAVE_XFIXES)
#include <X11/extensions/Xfixes.h>
#endif
#endif

#if defined(TOPSCREEN_HAVE_EVDEV)
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace topscreen {

namespace {

struct LinuxInput {
#if defined(TOPSCREEN_HAVE_X11)
	Display *dpy = nullptr;
	unsigned char prev[32] = {};
	bool xfixes = false;
	int link_poll = 0;
	bool link = false;
#endif
#if defined(TOPSCREEN_HAVE_EVDEV)
	int fd = -1;
	unsigned char buf[1024] = {};
	size_t buflen = 0;
	bool shift = false;
	bool altgr = false;
	bool ctrl = false;
	bool alt = false;
	bool meta = false;
#endif
};

#if defined(TOPSCREEN_HAVE_X11)
std::string utf8_from_keysym(KeySym ks)
{
	if (ks >= 0x20 && ks <= 0x7E) {
		char c = static_cast<char>(ks);
		if (std::isalpha(static_cast<unsigned char>(c)))
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		return std::string(1, c);
	}
	if (ks >= 0xA0 && ks <= 0xFF) {
		char buf[2];
		buf[0] = static_cast<char>(0xC0 | (ks >> 6));
		buf[1] = static_cast<char>(0x80 | (ks & 0x3F));
		return std::string(buf, 2);
	}
	return {};
}

std::string x11_label(KeySym ks, bool *is_mod)
{
	*is_mod = false;

	if (ks == XK_space)
		return "Space";
	if (const std::string text = utf8_from_keysym(ks); !text.empty())
		return text;

	char *name = XKeysymToString(ks);
	if (!name)
		return {};

	std::string n(name);
	for (char &c : n)
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

	if (n == "RETURN")
		return "Enter";
	if (n == "BACKSPACE")
		return "Backspace";
	if (n == "ESCAPE")
		return "Esc";
	if (n == "SPACE")
		return "Space";
	if (n == "TAB")
		return "Tab";
	if (n == "LEFT")
		return "Left";
	if (n == "RIGHT")
		return "Right";
	if (n == "UP")
		return "Up";
	if (n == "DOWN")
		return "Down";
	if (n.rfind("CONTROL", 0) == 0) {
		*is_mod = true;
		return "Ctrl";
	}
	if (n.rfind("SHIFT", 0) == 0) {
		*is_mod = true;
		return "Shift";
	}
	if (n.rfind("ALT", 0) == 0) {
		*is_mod = true;
		return "Alt";
	}
	if (n.rfind("SUPER", 0) == 0 || n.rfind("META", 0) == 0) {
		*is_mod = true;
		return "Win";
	}
	if (n.size() == 1 && std::isalnum(static_cast<unsigned char>(n[0])))
		return n;

	return {};
}

std::string x11_label_from_keycode(Display *dpy, int evdev_code, bool shift, bool altgr)
{
	if (!dpy || evdev_code <= 0)
		return {};
	const KeyCode kc = static_cast<KeyCode>(evdev_code + 8);
	const int level = (shift ? 1 : 0) + (altgr ? 2 : 0);
	const KeySym ks = XkbKeycodeToKeysym(dpy, kc, 0, level);
	if (ks == NoSymbol)
		return {};
	bool is_mod = false;
	return x11_label(ks, &is_mod);
}

#if defined(TOPSCREEN_HAVE_XFIXES)
bool x11_cursor_is_link(Display *dpy)
{
	if (!dpy)
		return false;
	XFixesCursorImage *img = XFixesGetCursorImage(dpy);
	if (!img)
		return false;

	const bool link = img->name && core::cursor_name_is_link(img->name);
	XFree(img);
	return link;
}
#endif

uint32_t x11_modifiers(Display *dpy)
{
	if (!dpy)
		return 0;

	/* XKB returns all modifier masks in one call; the old path needed one
	 * XKeysymToKeycode() per modifier on every poll. */
	XkbStateRec state;
	if (XkbGetState(dpy, XkbUseCoreKbd, &state) == Success) {
		uint32_t mods = 0;
		if (state.mods & ShiftMask)
			mods |= core::KeyModShift;
		if (state.mods & ControlMask)
			mods |= core::KeyModCtrl;
		if (state.mods & Mod1Mask)
			mods |= core::KeyModAlt;
		if (state.mods & Mod4Mask)
			mods |= core::KeyModWin;
		return mods;
	}
	return 0;
}
#endif

#if defined(TOPSCREEN_HAVE_EVDEV)
bool evdev_is_keyboard(int fd)
{
	unsigned long ev_bits = 0;
	if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), &ev_bits) < 0)
		return false;
	if (!(ev_bits & (1UL << EV_KEY)))
		return false;

	unsigned long key_bits[(KEY_MAX / 8) + 1] = {};
	if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0)
		return false;

	auto has = [&](int code) { return (key_bits[code / 8] & (1UL << (code % 8))) != 0; };
	return has(KEY_A) && has(KEY_Z) && has(KEY_SPACE);
}

int evdev_open_keyboard()
{
	for (int i = 0; i < 32; ++i) {
		char path[32];
		std::snprintf(path, sizeof(path), "/dev/input/event%d", i);

		const int fd = ::open(path, O_RDONLY | O_NONBLOCK);
		if (fd < 0)
			continue;

		if (evdev_is_keyboard(fd)) {
			blog(LOG_INFO, "[TopScreen] evdev: using %s", path);
			return fd;
		}
		::close(fd);
	}
	return -1;
}
#endif

} // namespace

void *ts_input_platform_start(obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	auto *s = new LinuxInput();

#if defined(TOPSCREEN_HAVE_X11)
	s->dpy = XOpenDisplay(nullptr);
	if (!s->dpy) {
		blog(LOG_INFO, "[TopScreen] X11 unavailable (X11 cursor/rotation disabled)");
	}
#if defined(TOPSCREEN_HAVE_XFIXES)
	else {
		int event_base = 0;
		int error_base = 0;
		s->xfixes = XFixesQueryExtension(s->dpy, &event_base, &error_base);
		if (!s->xfixes)
			blog(LOG_INFO, "[TopScreen] XFixes unavailable (link-cursor detection disabled)");
	}
#endif
#endif

#if defined(TOPSCREEN_HAVE_EVDEV)
	s->fd = evdev_open_keyboard();
	if (s->fd < 0)
		blog(LOG_INFO, "[TopScreen] evdev unavailable (permission on /dev/input?)");
#endif

	return s;
}

void ts_input_platform_stop(void *state)
{
	auto *s = static_cast<LinuxInput *>(state);
#if defined(TOPSCREEN_HAVE_X11)
	if (s->dpy)
		XCloseDisplay(s->dpy);
#endif
#if defined(TOPSCREEN_HAVE_EVDEV)
	if (s->fd >= 0)
		::close(s->fd);
#endif
	delete s;
}

void ts_input_platform_poll(void *state, InputState *out)
{
	auto *s = static_cast<LinuxInput *>(state);
	(void)s;

#if defined(TOPSCREEN_HAVE_X11)
	const bool x11 = (s->dpy != nullptr);
#else
	const bool x11 = false;
#endif

#if defined(TOPSCREEN_HAVE_EVDEV)
	const bool evdev = (s->fd >= 0);
#else
	const bool evdev = false;
#endif

	out->key_count = 0;

	/* Cursor and buttons (X11) */
#if defined(TOPSCREEN_HAVE_X11)
	if (x11) {
		Window root = DefaultRootWindow(s->dpy);
		Window child_ret;
		int root_x = 0, root_y = 0, win_x = 0, win_y = 0;
		unsigned int mask = 0;

		if (XQueryPointer(s->dpy, root, &child_ret, &child_ret, &root_x, &root_y, &win_x, &win_y,
				  &mask)) {
			out->x = static_cast<double>(root_x);
			out->y = static_cast<double>(root_y);
			out->available = true;

			out->left = (mask & Button1Mask) != 0;
			out->right = (mask & Button3Mask) != 0;
		}
		if (!evdev)
			out->modifiers = x11_modifiers(s->dpy);

#if defined(TOPSCREEN_HAVE_XFIXES)
		/* Cursor-shape lookup allocates; poll it at ~10 Hz instead of 60 Hz. */
		if (s->xfixes && (s->link_poll++ % 6) == 0)
			s->link = x11_cursor_is_link(s->dpy);
		out->link = s->link;
#endif
	}
#endif

	/* Keys: evdev has priority (it also works on Wayland). */
	bool keys_done = false;
	(void)keys_done;
#if defined(TOPSCREEN_HAVE_EVDEV)
	if (evdev) {
		for (;;) {
			const ssize_t n = ::read(s->fd, s->buf + s->buflen, sizeof(s->buf) - s->buflen);
			if (n <= 0)
				break;
			s->buflen += static_cast<size_t>(n);
			if (s->buflen >= sizeof(s->buf))
				break;
		}

		const size_t rec = sizeof(struct input_event);
		if (s->buflen >= rec) {
			const size_t usable = (s->buflen / rec) * rec;
			static thread_local std::vector<core::EvdevKey> events;
			events.clear();
			core::evdev_decode(s->buf, usable, rec, &events);
			std::memmove(s->buf, s->buf + usable, s->buflen - usable);
			s->buflen -= usable;

			for (const core::EvdevKey &k : events) {
				if (k.code == KEY_LEFTSHIFT || k.code == KEY_RIGHTSHIFT) {
					s->shift = k.pressed;
					continue;
				}
				if (k.code == KEY_LEFTCTRL || k.code == KEY_RIGHTCTRL) {
					s->ctrl = k.pressed;
					continue;
				}
				if (k.code == KEY_LEFTALT || k.code == KEY_RIGHTALT) {
					s->alt = k.pressed;
					if (k.code == KEY_RIGHTALT)
						s->altgr = k.pressed;
					continue;
				}
				if (k.code == KEY_LEFTMETA || k.code == KEY_RIGHTMETA) {
					s->meta = k.pressed;
					continue;
				}
				if (!k.pressed)
					continue;

				std::string label;
#if defined(TOPSCREEN_HAVE_X11)
				label = x11_label_from_keycode(s->dpy, k.code, s->shift, s->altgr);
#endif
				if (label.empty())
					label = core::evdev_key_label_with_shift(k.code, s->shift, s->altgr);
				if (label.empty())
					continue;
				if (out->key_count < InputState::kMaxKeyEvents) {
					out->key_labels[out->key_count] = label;
					out->key_modifier[out->key_count] = core::evdev_key_is_modifier(k.code);
					++out->key_count;
				}
			}
		}
		out->modifiers = 0;
		if (s->shift)
			out->modifiers |= core::KeyModShift;
		if (s->ctrl)
			out->modifiers |= core::KeyModCtrl;
		if (s->alt)
			out->modifiers |= core::KeyModAlt;
		if (s->meta)
			out->modifiers |= core::KeyModWin;
		keys_done = true;
	}
#endif

	/* X11 key fallback */
#if defined(TOPSCREEN_HAVE_X11)
	if (x11 && !keys_done) {
		char keys[32] = {};
		XQueryKeymap(s->dpy, keys);
		auto down = [&](KeyCode kc) {
			const unsigned char bit = static_cast<unsigned char>(1u << (kc & 7));
			return kc > 0 && (keys[kc >> 3] & bit) != 0;
		};
		const bool shift = down(XKeysymToKeycode(s->dpy, XK_Shift_L)) ||
				   down(XKeysymToKeycode(s->dpy, XK_Shift_R));
		for (int kc = 8; kc < 256; ++kc) {
			const unsigned char bit = static_cast<unsigned char>(1u << (kc & 7));
			const bool is_down = (keys[kc >> 3] & bit) != 0;
			const bool was = (s->prev[kc >> 3] & bit) != 0;
			if (is_down && !was) {
				const KeySym ks = XkbKeycodeToKeysym(s->dpy, static_cast<KeyCode>(kc), 0,
								      shift ? 1 : 0);
				if (ks != NoSymbol) {
					bool is_mod = false;
					const std::string label = x11_label(ks, &is_mod);
					if (!label.empty() && out->key_count < InputState::kMaxKeyEvents) {
						out->key_labels[out->key_count] = label;
						out->key_modifier[out->key_count] = is_mod;
						++out->key_count;
					}
				}
			}
		}
		std::memcpy(s->prev, keys, sizeof(keys));
	}
#endif

	/* Tier: 1 if there is a native cursor (X11); 3 if we only have keys via evdev. */
	if (x11)
		out->tier = 1;
	else if (evdev)
		out->tier = 3;
	else
		out->tier = 0;
}

} // namespace topscreen

#else

/* No native backend: input unavailable (graceful degradation - Tier 4). */

#include <cstdlib>

#include <string>

namespace topscreen {

void *ts_input_platform_start(obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	return reinterpret_cast<void *>(1);
}

void ts_input_platform_stop(void *state)
{
	UNUSED_PARAMETER(state);
}

void ts_input_platform_poll(void *state, InputState *out)
{
	UNUSED_PARAMETER(state);
	out->available = false;
	out->tier = 0;
	out->key_count = 0;

	/*
	 * Debug hook: with TOPSCREEN_DEBUG_KEY defined, injects that key
	 * periodically so the overlay can be tested without an input back-end.
	 */
	static int counter = 0;
	const char *debug = std::getenv("TOPSCREEN_DEBUG_KEY");
	if (debug && *debug && (counter++ % 60) == 0) {
		out->key_labels[0] = debug;
		out->key_modifier[0] = false;
		out->key_count = 1;
	}
}

} // namespace topscreen

#endif
