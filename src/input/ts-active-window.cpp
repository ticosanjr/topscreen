#include "ts-active-window.hpp"

#include <string>

#if defined(_WIN32)
#include <windows.h>

#include <cctype>
#elif defined(__linux__) && defined(TOPSCREEN_HAVE_X11)
#include <X11/Xlib.h>

#include <cstring>
#endif

namespace topscreen {

namespace {

[[maybe_unused]] bool contains_obs(const char *name)
{
	if (!name || !*name)
		return false;
	for (const char *p = name; *p; ++p) {
		if ((p[0] == 'O' || p[0] == 'o') && p[1] && p[2] &&
		    (p[1] == 'B' || p[1] == 'b') && (p[2] == 'S' || p[2] == 's'))
			return true;
	}
	return false;
}

} // namespace

void ActiveWindow::poll(double now_ms)
{
	if (now_ms - last_poll_ms_ < 500.0)
		return;
	last_poll_ms_ = now_ms;
	rect_ = {};

#if defined(_WIN32)
	HWND hwnd = GetForegroundWindow();
	if (!hwnd)
		return;
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == GetCurrentProcessId())
		return;

	RECT r = {};
	if (!GetWindowRect(hwnd, &r))
		return;
	const double w = static_cast<double>(r.right - r.left);
	const double h = static_cast<double>(r.bottom - r.top);
	if (w < 20.0 || h < 20.0)
		return;
	rect_.valid = true;
	rect_.x = static_cast<double>(r.left);
	rect_.y = static_cast<double>(r.top);
	rect_.width = w;
	rect_.height = h;
#elif defined(__linux__) && defined(TOPSCREEN_HAVE_X11)
	Display *dpy = XOpenDisplay(nullptr);
	if (!dpy)
		return;

	Window focus = 0;
	int revert = 0;
	XGetInputFocus(dpy, &focus, &revert);
	if (focus == None || focus == PointerRoot) {
		XCloseDisplay(dpy);
		return;
	}

	/* Walk up to the top-level client window. */
	Window root = DefaultRootWindow(dpy);
	Window win = focus;
	for (int depth = 0; depth < 12; ++depth) {
		Window root_ret = 0;
		Window parent = 0;
		Window *children = nullptr;
		unsigned int count = 0;
		if (!XQueryTree(dpy, win, &root_ret, &parent, &children, &count)) {
			win = 0;
			break;
		}
		if (children)
			XFree(children);
		if (!parent || parent == root)
			break;
		win = parent;
	}
	if (!win) {
		XCloseDisplay(dpy);
		return;
	}

	XWindowAttributes attr = {};
	if (!XGetWindowAttributes(dpy, win, &attr) || attr.width < 20 || attr.height < 20) {
		XCloseDisplay(dpy);
		return;
	}

	char *name = nullptr;
	if (XFetchName(dpy, win, &name) && name) {
		const bool ignore = contains_obs(name);
		XFree(name);
		if (ignore) {
			XCloseDisplay(dpy);
			return;
		}
	}

	int rx = 0;
	int ry = 0;
	Window child = 0;
	if (!XTranslateCoordinates(dpy, win, root, 0, 0, &rx, &ry, &child)) {
		XCloseDisplay(dpy);
		return;
	}

	rect_.valid = true;
	rect_.x = static_cast<double>(rx);
	rect_.y = static_cast<double>(ry);
	rect_.width = static_cast<double>(attr.width);
	rect_.height = static_cast<double>(attr.height);
	XCloseDisplay(dpy);
#else
	(void)now_ms;
#endif
}

} // namespace topscreen
