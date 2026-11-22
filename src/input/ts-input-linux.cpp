#include "ts-input.hpp"

/*
 * Platform input backend: Linux X11 for cursor/buttons (Tier 1) and evdev for
 * keys (Tier 3). Empty on every other platform.
 */

#if defined(__linux__)
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "core/cursor.hpp"
#include "core/evdev.hpp"
#include "core/key_modifiers.hpp"

#if defined(TOPSCREEN_HAVE_X11)
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
	KeyCode shift_l = 0;
	KeyCode shift_r = 0;
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
	if (s->dpy) {
		s->shift_l = XKeysymToKeycode(s->dpy, XK_Shift_L);
		s->shift_r = XKeysymToKeycode(s->dpy, XK_Shift_R);
	}
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

#if defined(TOPSCREEN_HAVE_X11)
void poll_x11_pointer(LinuxInput *s, InputState *out, bool evdev)
{

	if (s->dpy) {
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
}

void poll_x11_keys(LinuxInput *s, InputState *out)
{
	char keys[32] = {};
	XQueryKeymap(s->dpy, keys);
	auto down = [&](KeyCode kc) {
		const unsigned char bit = static_cast<unsigned char>(1u << (kc & 7));
		return kc > 0 && (keys[kc >> 3] & bit) != 0;
	};
	const bool shift = down(s->shift_l) || down(s->shift_r);
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
					out->key_modifiers[out->key_count] = out->modifiers;
					++out->key_count;
				}
			}
		}
	}
	std::memcpy(s->prev, keys, sizeof(keys));
}
#else
void poll_x11_pointer(LinuxInput *, InputState *, bool) {}
void poll_x11_keys(LinuxInput *, InputState *) {}
#endif

#if defined(TOPSCREEN_HAVE_EVDEV)
void emit_evdev_events(LinuxInput *s, InputState *out,
		       const std::vector<core::EvdevKey> &events)
{
	auto current_modifiers = [&]() {
		uint32_t mods = 0;
		if (s->shift)
			mods |= core::KeyModShift;
		if (s->ctrl)
			mods |= core::KeyModCtrl;
		if (s->alt)
			mods |= core::KeyModAlt;
		if (s->meta)
			mods |= core::KeyModWin;
		return mods;
	};
	auto emit = [&](std::string_view label, bool is_modifier) {
		if (label.empty() || out->key_count >= InputState::kMaxKeyEvents)
			return;
		out->key_labels[out->key_count] = std::string(label);
		out->key_modifier[out->key_count] = is_modifier;
		out->key_modifiers[out->key_count] = current_modifiers();
		++out->key_count;
	};

	for (const core::EvdevKey &k : events) {
		if (k.code == KEY_LEFTSHIFT || k.code == KEY_RIGHTSHIFT) {
			const bool was = s->shift;
			s->shift = k.pressed;
			if (k.pressed && !was)
				emit("Shift", true);
			continue;
		}
		if (k.code == KEY_LEFTCTRL || k.code == KEY_RIGHTCTRL) {
			const bool was = s->ctrl;
			s->ctrl = k.pressed;
			if (k.pressed && !was)
				emit("Ctrl", true);
			continue;
		}
		if (k.code == KEY_LEFTALT || k.code == KEY_RIGHTALT) {
			const bool was = s->alt;
			s->alt = k.pressed;
			if (k.code == KEY_RIGHTALT)
				s->altgr = k.pressed;
			if (k.pressed && !was)
				emit("Alt", true);
			continue;
		}
		if (k.code == KEY_LEFTMETA || k.code == KEY_RIGHTMETA) {
			const bool was = s->meta;
			s->meta = k.pressed;
			if (k.pressed && !was)
				emit("Win", true);
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
		emit(label, core::evdev_key_is_modifier(k.code));
	}
}

bool poll_evdev_keys(LinuxInput *s, InputState *out)
{
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

		emit_evdev_events(s, out, events);
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
	return true;
}
#else
bool poll_evdev_keys(LinuxInput *, InputState *) { return false; }
#endif

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
	if (x11)
		poll_x11_pointer(s, out, evdev);

	bool keys_done = false;
	if (evdev)
		keys_done = poll_evdev_keys(s, out);
	if (x11 && !keys_done)
		poll_x11_keys(s, out);

	/* Tier: 1 if there is a native cursor (X11); 3 if we only have keys via evdev. */
	if (x11)
		out->tier = 1;
	else if (evdev)
		out->tier = 3;
	else
		out->tier = 0;
}

} // namespace topscreen

#endif
