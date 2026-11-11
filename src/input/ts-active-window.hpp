#pragma once

/* Xlib's opaque display type, forward-declared so this header does not pull
 * X11 into every translation unit that only needs the poll interface. */
struct _XDisplay;

namespace topscreen {

/** Global desktop rectangle of the foreground window, when available. */
struct ActiveWindowRect {
	bool valid = false;
	double x = 0.0;
	double y = 0.0;
	double width = 0.0;
	double height = 0.0;
};

/**
 * Polls the foreground window for Smart Zoom. Windows uses GetForegroundWindow;
 * Linux/X11 uses the input focus and translates it to root coordinates.
 * Unsupported platforms simply return an invalid rect.
 *
 * The X11 display connection is kept open for the lifetime of the object: the
 * poll runs on the video thread, so reopening the display every 500 ms would
 * add avoidable I/O latency.
 */
class ActiveWindow {
public:
	ActiveWindow() = default;
	~ActiveWindow();

	ActiveWindow(const ActiveWindow &) = delete;
	ActiveWindow &operator=(const ActiveWindow &) = delete;

	void poll(double now_ms);
	const ActiveWindowRect &rect() const { return rect_; }

private:
	void close_display();
	double last_poll_ms_ = -1.0e9;
	ActiveWindowRect rect_;

#if defined(__linux__) && defined(TOPSCREEN_HAVE_X11)
	::_XDisplay *display_ = nullptr;
#endif
};

} // namespace topscreen
