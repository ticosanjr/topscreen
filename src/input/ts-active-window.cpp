#include "ts-active-window.hpp"

#if defined(_WIN32)
#include <windows.h>

#include <dwmapi.h>
#elif defined(__linux__) && defined(TOPSCREEN_HAVE_X11)
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <cstdlib>
#include <unistd.h>
#endif

namespace topscreen {

namespace {

#if defined(__linux__) && defined(TOPSCREEN_HAVE_X11)

bool read_cardinal(Display *dpy, Window win, const char *property, unsigned long *value)
{
	const Atom atom = XInternAtom(dpy, property, True);
	if (atom == None)
		return false;

	Atom type = None;
	int format = 0;
	unsigned long count = 0;
	unsigned long after = 0;
	unsigned char *data = nullptr;
	if (XGetWindowProperty(dpy, win, atom, 0, 1, False, XA_CARDINAL, &type, &format,
			       &count, &after, &data) != Success ||
	    !data)
		return false;

	const bool ok = type == XA_CARDINAL && format == 32 && count >= 1;
	if (ok)
		*value = *reinterpret_cast<unsigned long *>(data);
	XFree(data);
	return ok;
}

bool ignored_window_type(Display *dpy, Window win)
{
	const Atom property = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", True);
	if (property == None)
		return false;

	Atom type = None;
	int format = 0;
	unsigned long count = 0;
	unsigned long after = 0;
	unsigned char *data = nullptr;
	if (XGetWindowProperty(dpy, win, property, 0, 16, False, XA_ATOM, &type, &format,
			       &count, &after, &data) != Success ||
	    !data)
		return false;

	const char *const ignored[] = {
		"_NET_WM_WINDOW_TYPE_DOCK",    "_NET_WM_WINDOW_TYPE_DESKTOP",
		"_NET_WM_WINDOW_TYPE_TOOLBAR", "_NET_WM_WINDOW_TYPE_MENU",
		"_NET_WM_WINDOW_TYPE_SPLASH",  "_NET_WM_WINDOW_TYPE_NOTIFICATION",
	};

	bool skip = false;
	if (type == XA_ATOM && format == 32) {
		const Atom *atoms = reinterpret_cast<const Atom *>(data);
		for (unsigned long i = 0; i < count && !skip; ++i) {
			for (const char *name : ignored) {
				const Atom candidate = XInternAtom(dpy, name, True);
				if (candidate != None && atoms[i] == candidate) {
					skip = true;
					break;
				}
			}
		}
	}
	XFree(data);
	return skip;
}

void top_level_window(Display *dpy, Window focus, Window root, Window *out)
{
	Window win = focus;
	for (int depth = 0; depth < 12; ++depth) {
		Window root_ret = 0;
		Window parent = 0;
		Window *children = nullptr;
		unsigned int count = 0;
		if (!XQueryTree(dpy, win, &root_ret, &parent, &children, &count)) {
			*out = 0;
			return;
		}
		if (children)
			XFree(children);
		if (!parent || parent == root)
			break;
		win = parent;
	}
	*out = win;
}

#endif

} // namespace

ActiveWindow::~ActiveWindow()
{
	close_display();
}

void ActiveWindow::close_display()
{
#if defined(__linux__) && defined(TOPSCREEN_HAVE_X11)
	if (display_) {
		XCloseDisplay(reinterpret_cast<Display *>(display_));
		display_ = nullptr;
	}
#endif
}

void ActiveWindow::poll(double now_ms)
{
	if (last_poll_ms_ >= 0.0 && now_ms >= last_poll_ms_ &&
	    now_ms - last_poll_ms_ < 500.0)
		return;
	last_poll_ms_ = now_ms;
	rect_ = {};

#if defined(_WIN32)
	HWND hwnd = GetForegroundWindow();
	if (!hwnd || IsIconic(hwnd))
		return;
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == GetCurrentProcessId())
		return;

	RECT r = {};
	/* GetWindowRect includes the invisible DWM resize border on Windows 10+,
	 * which would make Smart Zoom focus a slightly larger area than the one
	 * the user sees. Prefer the extended frame bounds when available. */
	const HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r,
						 sizeof(r));
	const bool dwm_ok = SUCCEEDED(hr) && r.right > r.left && r.bottom > r.top;
	if (!dwm_ok && !GetWindowRect(hwnd, &r))
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
	if (!display_)
		display_ = XOpenDisplay(nullptr);
	Display *dpy = reinterpret_cast<Display *>(display_);
	if (!dpy)
		return;

	Window focus = 0;
	int revert = 0;
	XGetInputFocus(dpy, &focus, &revert);
	if (focus == None || focus == PointerRoot)
		return;

	const Window root = DefaultRootWindow(dpy);
	Window win = 0;
	top_level_window(dpy, focus, root, &win);
	if (!win)
		return;

	unsigned long pid = 0;
	if (read_cardinal(dpy, win, "_NET_WM_PID", &pid) &&
	    pid == static_cast<unsigned long>(getpid()))
		return;
	if (ignored_window_type(dpy, win))
		return;

	XWindowAttributes attr = {};
	if (!XGetWindowAttributes(dpy, win, &attr) || attr.width < 20 || attr.height < 20)
		return;

	int rx = 0;
	int ry = 0;
	Window child = 0;
	if (!XTranslateCoordinates(dpy, win, root, 0, 0, &rx, &ry, &child))
		return;

	rect_.valid = true;
	rect_.x = static_cast<double>(rx);
	rect_.y = static_cast<double>(ry);
	rect_.width = static_cast<double>(attr.width);
	rect_.height = static_cast<double>(attr.height);
#else
	(void)now_ms;
#endif
}

} // namespace topscreen
