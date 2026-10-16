#include "ts-input.hpp"

/*
 * Backends de input por plataforma.
 *
 * Tier 1 (nativo):
 *   - Windows: WinAPI (GetCursorPos + GetAsyncKeyState)
 *   - X11:     Xlib (XQueryPointer)
 * Tier 2 (portal GlobalShortcuts) e Tier 3 (evdev) entram em F7; ver
 * docs/INPUT-E-PLATAFORMAS.md.
 */

#if defined(_WIN32)

#include <windows.h>

namespace topscreen {

namespace {
struct WinInput {
	bool ok = false;
};
} // namespace

void *ts_input_platform_start(obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	auto *s = new WinInput();
	s->ok = true;
	return s;
}

void ts_input_platform_stop(void *state)
{
	delete static_cast<WinInput *>(state);
}

void ts_input_platform_poll(void *state, InputState *out)
{
	UNUSED_PARAMETER(state);

	POINT pt;
	if (GetCursorPos(&pt)) {
		out->x = static_cast<double>(pt.x);
		out->y = static_cast<double>(pt.y);
		out->available = true;
		out->tier = 1;
	}

	out->left = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	out->right = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

} // namespace topscreen

#elif defined(TOPSCREEN_HAVE_X11)

#include <X11/Xlib.h>

namespace topscreen {

namespace {
struct X11Input {
	Display *dpy = nullptr;
};
} // namespace

void *ts_input_platform_start(obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	auto *s = new X11Input();
	s->dpy = XOpenDisplay(nullptr);
	if (!s->dpy)
		blog(LOG_WARNING, "[TopScreen] nao foi possivel abrir o display X11");
	return s;
}

void ts_input_platform_stop(void *state)
{
	auto *s = static_cast<X11Input *>(state);
	if (s->dpy)
		XCloseDisplay(s->dpy);
	delete s;
}

void ts_input_platform_poll(void *state, InputState *out)
{
	auto *s = static_cast<X11Input *>(state);
	if (!s->dpy)
		return;

	Window root = DefaultRootWindow(s->dpy);
	Window child_ret;
	int root_x = 0, root_y = 0, win_x = 0, win_y = 0;
	unsigned int mask = 0;

	if (XQueryPointer(s->dpy, root, &child_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask)) {
		out->x = static_cast<double>(root_x);
		out->y = static_cast<double>(root_y);
		out->available = true;
		out->tier = 1;

		out->left = (mask & Button1Mask) != 0;
		out->right = (mask & Button3Mask) != 0;
	}
}

} // namespace topscreen

#else

/* Sem backend nativo: input indisponivel (degradacao graciosa - Tier 4). */

namespace topscreen {

void *ts_input_platform_start(obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	return nullptr;
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
}

} // namespace topscreen

#endif
