#pragma once

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
 */
class ActiveWindow {
public:
	void poll(double now_ms);
	const ActiveWindowRect &rect() const { return rect_; }

private:
	double last_poll_ms_ = -1.0e9;
	ActiveWindowRect rect_;
};

} // namespace topscreen
