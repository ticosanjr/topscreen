#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace topscreen::capture {

/** One physical monitor in global desktop coordinates. */
struct MonitorInfo {
	int index = 0;
	int x = 0;
	int y = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	std::string name;
	bool primary = false;
};

enum class PixelFormat {
	Bgra,
	Rgba,
};

/** A captured frame borrowed from the backend until the next capture call. */
struct Frame {
	const uint8_t *data = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t stride = 0;
	PixelFormat format = PixelFormat::Bgra;
};

struct Config {
	int monitor = 0;
	/* Reserved for backends that can include the native cursor. TopScreen
	 * usually draws its own cursor, so the default is off. */
	bool capture_cursor = false;
};

/**
 * Platform capture backend used by the future TopScreen source.
 *
 * The interface is intentionally small: enumerate monitors, start on one of
 * them and pull the latest frame. The OBS layer owns the GPU texture upload
 * and the effect pipeline; this module only produces pixels.
 */
class Backend {
public:
	Backend() = default;
	virtual ~Backend() = default;

	Backend(const Backend &) = delete;
	Backend &operator=(const Backend &) = delete;

	virtual const char *id() const = 0;
	virtual bool start(const Config &config) = 0;
	virtual void stop() = 0;
	/** Toggle native-cursor compositing without restarting the backend. */
	virtual void set_capture_cursor(bool enabled) = 0;
	/** Output size of the selected monitor (0 when not started). */
	virtual void size(uint32_t *width, uint32_t *height) const = 0;
	/** Non-blocking: returns false when no new frame is available. */
	virtual bool capture(Frame *frame) = 0;
};

/** Monitors available to the current platform (global coordinates). */
std::vector<MonitorInfo> enumerate_monitors();

/** Best backend for this platform; never null. */
std::unique_ptr<Backend> create_backend();

} // namespace topscreen::capture
