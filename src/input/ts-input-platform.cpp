#include "ts-input.hpp"

/*
 * Backends de input por plataforma.
 *
 *   - Windows: WinAPI (cursor + botoes + scan de teclas)  -> Tier 1
 *   - Linux:   X11 para cursor/botoes (Tier 1) e evdev para teclas (Tier 3)
 *   - Sem backend: degradacao graciosa (Tier 4)
 *
 * Tier 2 (portal GlobalShortcuts) entra em F7; ver docs/INPUT-E-PLATAFORMAS.md.
 */

#if defined(_WIN32)

#include <windows.h>

#include <string>

namespace topscreen {

namespace {

struct WinInput {
	unsigned char prev[256] = {};
};

std::string win_key_label(int vk, bool *is_mod)
{
	*is_mod = false;

	if (vk >= 'A' && vk <= 'Z')
		return std::string(1, static_cast<char>(vk));
	if (vk >= '0' && vk <= '9')
		return std::string(1, static_cast<char>(vk));

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
		return {};
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

	for (int vk = 0; vk < 256; ++vk) {
		const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
		if (down && !s->prev[vk]) {
			bool is_mod = false;
			const std::string label = win_key_label(vk, &is_mod);
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

#include "core/evdev.hpp"

#if defined(TOPSCREEN_HAVE_X11)
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <cctype>
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
#endif
#if defined(TOPSCREEN_HAVE_EVDEV)
	int fd = -1;
	unsigned char buf[1024] = {};
	size_t buflen = 0;
#endif
};

#if defined(TOPSCREEN_HAVE_X11)
std::string x11_label(KeySym ks, bool *is_mod)
{
	*is_mod = false;

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
			blog(LOG_INFO, "[TopScreen] evdev: usando %s", path);
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
	if (!s->dpy)
		blog(LOG_INFO, "[TopScreen] X11 indisponivel (cursor/rotação por X11 desativados)");
#endif

#if defined(TOPSCREEN_HAVE_EVDEV)
	s->fd = evdev_open_keyboard();
	if (s->fd < 0)
		blog(LOG_INFO, "[TopScreen] evdev indisponivel (permissao em /dev/input?)");
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

	/* Cursor e botoes (X11) */
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
	}
#endif

	/* Teclas: evdev tem prioridade (funciona tambem no Wayland). */
	bool keys_done = false;
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
				if (!k.pressed)
					continue;
				const std::string label = core::evdev_key_label(k.code);
				if (label.empty())
					continue;
				if (out->key_count < InputState::kMaxKeyEvents) {
					out->key_labels[out->key_count] = label;
					out->key_modifier[out->key_count] = core::evdev_key_is_modifier(k.code);
					++out->key_count;
				}
			}
		}
		keys_done = true;
	}
#endif

	/* Fallback de teclas por X11 */
#if defined(TOPSCREEN_HAVE_X11)
	if (x11 && !keys_done) {
		char keys[32] = {};
		XQueryKeymap(s->dpy, keys);
		for (int kc = 8; kc < 256; ++kc) {
			const unsigned char bit = static_cast<unsigned char>(1u << (kc & 7));
			const bool down = (keys[kc >> 3] & bit) != 0;
			const bool was = (s->prev[kc >> 3] & bit) != 0;
			if (down && !was) {
				const KeySym ks = XkbKeycodeToKeysym(s->dpy, static_cast<KeyCode>(kc), 0, 0);
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

	/* Tier: 1 se ha cursor nativo (X11); 3 se so temos teclas via evdev. */
	if (x11)
		out->tier = 1;
	else if (evdev)
		out->tier = 3;
	else
		out->tier = 0;
}

} // namespace topscreen

#else

/* Sem backend nativo: input indisponivel (degradacao graciosa - Tier 4). */

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
	 * Gancho de depuracao: com TOPSCREEN_DEBUG_KEY definida, injeta essa tecla
	 * periodicamente para que o overlay possa ser testado sem back-end de input.
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
