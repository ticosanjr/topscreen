#include "ts-monitor.hpp"

#include <obs-module.h>

#include <cstdlib>
#include <cstring>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#if defined(TOPSCREEN_HAVE_X11)
#include <X11/Xlib.h>
#endif
#endif

namespace topscreen {

namespace {

constexpr int kMaxMonitors = 16;

bool parse_pair(const char *text, double *x, double *y)
{
	if (!text || !*text)
		return false;
	char *end = nullptr;
	const double vx = std::strtod(text, &end);
	if (end == text)
		return false;
	while (*end == ' ' || *end == ',' || *end == ';')
		++end;
	const double vy = std::strtod(end, &end);
	if (end == text)
		return false;
	*x = vx;
	*y = vy;
	return true;
}

#if defined(_WIN32)

/* Index of the monitor whose device name matches (e.g. "\\.\DISPLAY2"). */
struct DeviceSearch {
	const char *device = nullptr;
	int index = -1;
	int seen = 0;
};

BOOL CALLBACK device_proc(HMONITOR mon, HDC, LPRECT, LPARAM data)
{
	auto *ctx = reinterpret_cast<DeviceSearch *>(data);
	MONITORINFOEXW info{};
	info.cbSize = sizeof(info);
	if (GetMonitorInfoW(mon, &info)) {
		char name[64] = {};
		WideCharToMultiByte(CP_ACP, 0, info.szDevice, -1, name, sizeof(name) - 1, nullptr,
				    nullptr);
		if (ctx->device && _stricmp(name, ctx->device) == 0 && ctx->index < 0)
			ctx->index = ctx->seen;
		++ctx->seen;
	}
	return TRUE;
}

BOOL CALLBACK enum_proc(HMONITOR mon, HDC, LPRECT, LPARAM data)
{
	auto *ctx = reinterpret_cast<std::pair<core::Monitor *, int> *>(data);
	if (ctx->second >= kMaxMonitors)
		return FALSE;

	MONITORINFO info{};
	info.cbSize = sizeof(info);
	if (GetMonitorInfoW(mon, &info)) {
		core::Monitor &m = ctx->first[ctx->second++];
		m.x = static_cast<double>(info.rcMonitor.left);
		m.y = static_cast<double>(info.rcMonitor.top);
		m.width = static_cast<double>(info.rcMonitor.right - info.rcMonitor.left);
		m.height = static_cast<double>(info.rcMonitor.bottom - info.rcMonitor.top);
	}
	return TRUE;
}

#elif defined(__linux__)

/* Xinerama is loaded at runtime: no build/link dependency on the dev package,
 * but it still gives the monitor rectangles in root-window coordinates (the
 * same space XQueryPointer reports). */
struct XineramaScreenInfoLocal {
	int screen_number;
	short x_org;
	short y_org;
	short width;
	short height;
};

#if defined(TOPSCREEN_HAVE_X11)
using xinerama_is_active_fn = int (*)(Display *);
using xinerama_query_fn = XineramaScreenInfoLocal *(*)(Display *, int *);

/*
 * The monitor origin is refreshed every ~10 s. Reopening the X connection
 * (and reloading the library) on every refresh is pointless: keep one display
 * per thread. thread_local also preserves Xlib's single-thread rule.
 */
struct XineramaCache {
	void *lib = nullptr;
	Display *dpy = nullptr;
	xinerama_is_active_fn is_active = nullptr;
	xinerama_query_fn query = nullptr;

	~XineramaCache()
	{
		if (dpy)
			XCloseDisplay(dpy);
		if (lib)
			dlclose(lib);
	}

	bool load()
	{
		if (!lib) {
			lib = dlopen("libXinerama.so.1", RTLD_LAZY | RTLD_LOCAL);
			if (!lib)
				lib = dlopen("libXinerama.so", RTLD_LAZY | RTLD_LOCAL);
			if (!lib)
				return false;
		}

		if (!is_active || !query) {
			is_active = reinterpret_cast<xinerama_is_active_fn>(
				dlsym(lib, "XineramaIsActive"));
			query = reinterpret_cast<xinerama_query_fn>(
				dlsym(lib, "XineramaQueryScreens"));
			if (!is_active || !query) {
				dlclose(lib);
				lib = nullptr;
				is_active = nullptr;
				query = nullptr;
				return false;
			}
		}

		if (!dpy)
			dpy = XOpenDisplay(nullptr);
		return dpy != nullptr;
	}
};
#endif

#endif

} // namespace

int ts_query_monitors(core::Monitor *out, int max)
{
	if (!out || max <= 0)
		return 0;
	if (max > kMaxMonitors)
		max = kMaxMonitors;

#if defined(_WIN32)
	std::pair<core::Monitor *, int> ctx{out, 0};
	EnumDisplayMonitors(nullptr, nullptr, enum_proc, reinterpret_cast<LPARAM>(&ctx));
	return ctx.second;

#elif defined(__linux__) && defined(TOPSCREEN_HAVE_X11)
	static thread_local XineramaCache cache;
	if (!cache.load())
		return 0;

	int count = 0;
	int written = 0;
	if (cache.is_active(cache.dpy)) {
		XineramaScreenInfoLocal *screens = cache.query(cache.dpy, &count);
		if (screens) {
			for (int i = 0; i < count && written < max; ++i) {
				core::Monitor &m = out[written++];
				m.x = screens[i].x_org;
				m.y = screens[i].y_org;
				m.width = screens[i].width;
				m.height = screens[i].height;
			}
			XFree(screens);
		}
	}

	return written;

#else
	UNUSED_PARAMETER(out);
	UNUSED_PARAMETER(max);
	return 0;
#endif
}

int ts_monitor_index_for_device(const char *device)
{
#if defined(_WIN32)
	if (!device || !*device)
		return -1;
	DeviceSearch ctx;
	ctx.device = device;
	EnumDisplayMonitors(nullptr, nullptr, device_proc, reinterpret_cast<LPARAM>(&ctx));
	return ctx.index;
#else
	UNUSED_PARAMETER(device);
	return -1;
#endif
}

bool ts_monitor_origin_override(double *origin_x, double *origin_y)
{
	if (!origin_x || !origin_y)
		return false;

	const char *env = std::getenv("TOPSCREEN_MONITOR_ORIGIN");
	if (!env || !*env)
		return false;

	double x = 0.0, y = 0.0;
	if (!parse_pair(env, &x, &y))
		return false;

	*origin_x = x;
	*origin_y = y;
	return true;
}

bool ts_resolve_monitor(const char *device_id, int hint, bool trust_hint, uint32_t capture_w,
			uint32_t capture_h, core::Monitor *out)
{
	if (!out)
		return false;

	double ox = 0.0, oy = 0.0;
	if (ts_monitor_origin_override(&ox, &oy)) {
		out->x = ox;
		out->y = oy;
		out->width = static_cast<double>(capture_w);
		out->height = static_cast<double>(capture_h);
		blog(LOG_INFO, "[TopScreen] monitor origin: (%.0f,%.0f) [TOPSCREEN_MONITOR_ORIGIN]",
		     ox, oy);
		return true;
	}

	/* An exact device match (Windows: "\\\\.\\DISPLAY2") is stronger than any
	 * size heuristic and works even with identical monitors. */
	const int device_index = ts_monitor_index_for_device(device_id);
	if (device_index >= 0) {
		hint = device_index;
		trust_hint = true;
	}

	core::Monitor monitors[kMaxMonitors];
	const int count = ts_query_monitors(monitors, kMaxMonitors);

	/* The list is printed once: the user can copy these numbers into the
	 * manual Origin X/Y fields when auto-detection is not enough. */
	static bool logged_list = false;
	if (count > 0 && !logged_list) {
		logged_list = true;
		for (int i = 0; i < count; ++i)
			blog(LOG_INFO, "[TopScreen] monitor %d: %.0fx%.0f at (%.0f,%.0f)", i,
			     monitors[i].width, monitors[i].height, monitors[i].x, monitors[i].y);
	}

	core::Monitor picked;
	if (core::pick_monitor(monitors, count, hint, trust_hint, capture_w, capture_h, &picked)) {
		*out = picked;
		blog(LOG_INFO, "[TopScreen] monitor origin: (%.0f,%.0f) of %.0fx%.0f [%d monitor(s)]",
		     picked.x, picked.y, picked.width, picked.height, count);
		return true;
	}

	if (count > 0)
		blog(LOG_INFO, "[TopScreen] monitor origin unknown (%d monitor(s), capture %ux%u): "
			       "assuming (0,0); set TOPSCREEN_MONITOR_ORIGIN=x,y to override",
		     count, capture_w, capture_h);
	out->x = 0.0;
	out->y = 0.0;
	out->width = static_cast<double>(capture_w);
	out->height = static_cast<double>(capture_h);
	return false;
}

bool ts_resolve_monitor_origin(const char *device_id, int hint, bool trust_hint, uint32_t capture_w,
			       uint32_t capture_h, double *origin_x, double *origin_y)
{
	if (!origin_x || !origin_y)
		return false;
	core::Monitor monitor;
	const bool known = ts_resolve_monitor(device_id, hint, trust_hint, capture_w, capture_h,
					      &monitor);
	*origin_x = monitor.x;
	*origin_y = monitor.y;
	return known;
}

} // namespace topscreen
