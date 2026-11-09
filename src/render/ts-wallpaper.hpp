#pragma once

#include <graphics/graphics.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "core/image.hpp"

namespace topscreen {

/** Shared result between the filter and the wallpaper cache worker. */
struct WallpaperRequestResult {
	std::mutex mutex;
	bool ready = false;
	bool failed = false;
	std::shared_ptr<const core::Image> image;
};

/**
 * Per-filter wallpaper texture and request state.
 *
 * Decoding, resize and blur happen in the shared WallpaperCache worker. The
 * pipeline only uploads the finished CPU image on the render thread.
 */
class WallpaperPipeline {
public:
	WallpaperPipeline() = default;
	~WallpaperPipeline();

	WallpaperPipeline(const WallpaperPipeline &) = delete;
	WallpaperPipeline &operator=(const WallpaperPipeline &) = delete;

	void update(const char *raw, uint32_t target_w, uint32_t target_h, double blur);
	void release();

	gs_texture_t *texture() const { return texture_; }
	bool has_texture() const { return texture_ != nullptr; }
	float aspect() const { return aspect_; }

private:
	bool poll_pending();
	bool upload_texture(const core::Image &image);
	void release_texture();
	static std::string resolve_path(const char *raw);

	gs_texture_t *texture_ = nullptr;
	float aspect_ = 1.0f;
	std::string requested_;
	std::string resolved_;
	std::string pending_key_;
	std::string actual_key_;
	std::string failed_key_;
	std::string cached_key_;
	uint32_t cached_w_ = 0;
	uint32_t cached_h_ = 0;
	double cached_blur_ = -1.0;
	std::shared_ptr<WallpaperRequestResult> pending_;
};

} // namespace topscreen
