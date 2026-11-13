#include "ts-capture.hpp"

#if defined(__linux__) && defined(TOPSCREEN_HAVE_XSHM)
#include "core/coord.hpp"
#include "input/ts-monitor.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#if defined(TOPSCREEN_HAVE_XFIXES)
#include <X11/extensions/Xfixes.h>
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#endif

namespace topscreen::capture {

namespace {

class NoneBackend final : public Backend {
public:
	const char *id() const override { return "none"; }
	bool start(const Config &) override { return false; }
	void stop() override {}
	void set_capture_cursor(bool) override {}
	void size(uint32_t *width, uint32_t *height) const override
	{
		if (width)
			*width = 0;
		if (height)
			*height = 0;
	}
	bool capture(Frame *) override { return false; }
};

#if defined(__linux__) && defined(TOPSCREEN_HAVE_XSHM)

class X11Backend final : public Backend {
public:
	X11Backend() { shm_.shmid = -1; }
	~X11Backend() override { stop(); }

	const char *id() const override { return use_shm_ ? "x11-shm" : "x11"; }

	bool start(const Config &config) override
	{
		stop();

		dpy_ = XOpenDisplay(nullptr);
		if (!dpy_)
			return false;

		screen_ = DefaultScreen(dpy_);
		root_ = RootWindow(dpy_, screen_);
		capture_cursor_ = config.capture_cursor;

		core::Monitor monitors[16] = {};
		const int count = ts_query_monitors(monitors, 16);
		if (count > 0) {
			const int index = std::clamp(config.monitor, 0, count - 1);
			monitor_ = monitors[index];
		} else {
			monitor_.x = 0.0;
			monitor_.y = 0.0;
			monitor_.width = static_cast<double>(DisplayWidth(dpy_, screen_));
			monitor_.height = static_cast<double>(DisplayHeight(dpy_, screen_));
		}

		const uint32_t w = static_cast<uint32_t>(monitor_.width);
		const uint32_t h = static_cast<uint32_t>(monitor_.height);
		if (!w || !h) {
			stop();
			return false;
		}

		if (!create_shm_image(w, h))
			create_cpu_image(w, h);

		if (!image_) {
			stop();
			return false;
		}
		return true;
	}

	void stop() override
	{
		if (shm_attached_ && dpy_) {
			XShmDetach(dpy_, &shm_);
			shm_attached_ = false;
		}
		if (image_) {
			/* Shared memory belongs to shm_, not to the XImage allocator. */
			if (use_shm_)
				image_->data = nullptr;
			XDestroyImage(image_);
			image_ = nullptr;
		}
		if (shm_.shmaddr) {
			shmdt(shm_.shmaddr);
			shm_.shmaddr = nullptr;
		}
		if (shm_.shmid >= 0) {
			shmctl(shm_.shmid, IPC_RMID, nullptr);
			shm_.shmid = -1;
		}
		if (dpy_) {
			XCloseDisplay(dpy_);
			dpy_ = nullptr;
		}
		use_shm_ = false;
	}

	void set_capture_cursor(bool enabled) override { capture_cursor_ = enabled; }

	void size(uint32_t *width, uint32_t *height) const override
	{
		if (width)
			*width = image_ ? static_cast<uint32_t>(image_->width) : 0;
		if (height)
			*height = image_ ? static_cast<uint32_t>(image_->height) : 0;
	}

	bool capture(Frame *frame) override
	{
		if (!frame || !dpy_ || !image_)
			return false;

		const int x = static_cast<int>(monitor_.x);
		const int y = static_cast<int>(monitor_.y);
		const bool ok = use_shm_
			? XShmGetImage(dpy_, root_, image_, x, y, AllPlanes)
			: XGetSubImage(dpy_, root_, x, y, image_->width, image_->height,
				       AllPlanes, ZPixmap, image_, 0, 0) != nullptr;
		if (!ok)
			return false;
		if (capture_cursor_)
			composite_cursor();

		frame->data = reinterpret_cast<const uint8_t *>(image_->data);
		frame->width = static_cast<uint32_t>(image_->width);
		frame->height = static_cast<uint32_t>(image_->height);
		frame->stride = static_cast<uint32_t>(image_->bytes_per_line);
		frame->format = pixel_format(*image_);
		return true;
	}

private:
	void composite_cursor()
	{
#if defined(TOPSCREEN_HAVE_XFIXES)
		if (!dpy_ || !image_ || !image_->data || image_->bits_per_pixel != 32)
			return;
		XFixesCursorImage *cursor = XFixesGetCursorImage(dpy_);
		if (!cursor)
			return;

		const int left = static_cast<int>(monitor_.x);
		const int top = static_cast<int>(monitor_.y);
		const int rx = static_cast<int>(cursor->x) - static_cast<int>(cursor->xhot);
		const int ry = static_cast<int>(cursor->y) - static_cast<int>(cursor->yhot);
		const int cw = static_cast<int>(cursor->width);
		const int ch = static_cast<int>(cursor->height);
		const PixelFormat format = pixel_format(*image_);

		for (int cy = 0; cy < ch; ++cy) {
			const int dy = ry + cy - top;
			if (dy < 0 || dy >= image_->height)
				continue;
			auto *row = reinterpret_cast<uint8_t *>(
				image_->data + static_cast<size_t>(dy) * image_->bytes_per_line);
			for (int cx = 0; cx < cw; ++cx) {
				const int dx = rx + cx - left;
				if (dx < 0 || dx >= image_->width)
					continue;
				const unsigned long p = cursor->pixels[cy * cw + cx];
				const unsigned int a = (p >> 24) & 0xffu;
				if (!a)
					continue;
				const unsigned int sr = (p >> 16) & 0xffu;
				const unsigned int sg = (p >> 8) & 0xffu;
				const unsigned int sb = p & 0xffu;
				uint8_t *dst = row + static_cast<size_t>(dx) * 4;
				auto blend = [a](uint8_t d, unsigned int s) {
					return static_cast<uint8_t>(
						(s * a + d * (255u - a) + 127u) / 255u);
				};
				if (format == PixelFormat::Bgra) {
					dst[0] = blend(dst[0], sb);
					dst[1] = blend(dst[1], sg);
					dst[2] = blend(dst[2], sr);
				} else {
					dst[0] = blend(dst[0], sr);
					dst[1] = blend(dst[1], sg);
					dst[2] = blend(dst[2], sb);
				}
				dst[3] = 0xff;
			}
		}
		XFree(cursor);
#else
		/* XFixes is optional at build time. */
#endif
	}

	static PixelFormat pixel_format(const XImage &image)
	{
		/* TrueColor masks tell us whether the first byte is B or R. Most
		 * little-endian X servers are BGRA, but do not assume it. */
		if (image.red_mask == 0x000000ffu && image.blue_mask == 0x00ff0000u)
			return PixelFormat::Rgba;
		return PixelFormat::Bgra;
	}

	bool create_shm_image(uint32_t w, uint32_t h)
	{
		if (!XShmQueryExtension(dpy_))
			return false;

		shm_.shmid = shmget(IPC_PRIVATE, static_cast<size_t>(w) * h * 4,
				    IPC_CREAT | 0600);
		if (shm_.shmid < 0)
			return false;

		shm_.shmaddr = static_cast<char *>(shmat(shm_.shmid, nullptr, 0));
		if (shm_.shmaddr == reinterpret_cast<char *>(-1)) {
			shm_.shmaddr = nullptr;
			shmctl(shm_.shmid, IPC_RMID, nullptr);
			shm_.shmid = -1;
			return false;
		}
		shm_.readOnly = False;

		image_ = XShmCreateImage(dpy_, DefaultVisual(dpy_, screen_),
					 static_cast<unsigned int>(DefaultDepth(dpy_, screen_)),
					 ZPixmap, shm_.shmaddr, &shm_, w, h);
		if (!image_) {
			shmdt(shm_.shmaddr);
			shm_.shmaddr = nullptr;
			shmctl(shm_.shmid, IPC_RMID, nullptr);
			shm_.shmid = -1;
			return false;
		}

		if (!XShmAttach(dpy_, &shm_)) {
			image_->data = nullptr;
			XDestroyImage(image_);
			image_ = nullptr;
			shmdt(shm_.shmaddr);
			shm_.shmaddr = nullptr;
			shmctl(shm_.shmid, IPC_RMID, nullptr);
			shm_.shmid = -1;
			return false;
		}
		XSync(dpy_, False);
		shm_attached_ = true;
		use_shm_ = true;
		return true;
	}

	void create_cpu_image(uint32_t w, uint32_t h)
	{
		image_ = XCreateImage(dpy_, DefaultVisual(dpy_, screen_),
				      static_cast<unsigned int>(DefaultDepth(dpy_, screen_)),
				      ZPixmap, 0, nullptr, w, h, 32, 0);
		if (!image_)
			return;

		const size_t bytes = static_cast<size_t>(image_->bytes_per_line) * h;
		image_->data = static_cast<char *>(std::calloc(bytes, 1));
		if (!image_->data) {
			XDestroyImage(image_);
			image_ = nullptr;
		}
	}

	Display *dpy_ = nullptr;
	int screen_ = 0;
	Window root_ = 0;
	core::Monitor monitor_;
	XImage *image_ = nullptr;
	XShmSegmentInfo shm_ = {};
	bool shm_attached_ = false;
	bool use_shm_ = false;
	std::atomic<bool> capture_cursor_{false};
};

#endif

} // namespace

std::vector<MonitorInfo> enumerate_monitors()
{
	std::vector<MonitorInfo> out;
#if defined(__linux__) && defined(TOPSCREEN_HAVE_XSHM)
	core::Monitor monitors[16] = {};
	const int count = ts_query_monitors(monitors, 16);
	out.reserve(count > 0 ? count : 0);
	for (int i = 0; i < count; ++i) {
		MonitorInfo info;
		info.index = i;
		info.x = static_cast<int>(monitors[i].x);
		info.y = static_cast<int>(monitors[i].y);
		info.width = static_cast<uint32_t>(monitors[i].width);
		info.height = static_cast<uint32_t>(monitors[i].height);
		info.name = "Monitor " + std::to_string(i + 1);
		info.primary = i == 0;
		out.push_back(std::move(info));
	}
#endif
	return out;
}

std::unique_ptr<Backend> create_backend()
{
#if defined(__linux__) && defined(TOPSCREEN_HAVE_XSHM)
	return std::make_unique<X11Backend>();
#else
	return std::make_unique<NoneBackend>();
#endif
}

} // namespace topscreen::capture
