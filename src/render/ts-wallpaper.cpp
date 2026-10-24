#include "ts-wallpaper.hpp"

#include <obs-module.h>

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "core/file.hpp"
#include "core/image_filter.hpp"
#include "ts-wallpapers.hpp"

namespace topscreen {

namespace {

std::string make_key(const std::string &path, uint32_t target_w, uint32_t target_h, double blur,
		     int blur_type)
{
	std::string key = path;
	key += '\n';
	key += std::to_string(target_w);
	key += '\n';
	key += std::to_string(target_h);
	key += '\n';
	key += std::to_string(static_cast<int>(blur * 100.0));
	key += '\n';
	key += std::to_string(blur_type);
	return key;
}

core::Image compute_wallpaper(const std::string &path, uint32_t target_w, uint32_t target_h,
			      double blur, int blur_type)
{
	std::vector<uint8_t> bytes;
	if (path.empty() || !core::read_file_bytes(path.c_str(), &bytes))
		return {};

	core::Image decoded;
	std::string error;
	if (!core::image_decode(bytes.data(), bytes.size(), &decoded, &error))
		return {};

	/* Bound the CPU cache for custom 8K images. */
	core::Image limited = core::image_limit_max(decoded, 4096);
	if (blur > 0.01 && blur_type > 0)
		return core::image_prefilter(limited, target_w, target_h, blur, blur_type);
	return limited;
}

/**
 * Shared CPU cache and worker.
 *
 * Jobs are coalesced by key: if two filters request the same wallpaper with
 * the same blur and target, both wait on one decode/filter job. Results are
 * shared_ptr images, so filters can upload independently without copying.
 */
class WallpaperCache {
public:
	static WallpaperCache &instance()
	{
		static WallpaperCache cache;
		return cache;
	}

	void request(const std::string &key, const std::string &path, uint32_t target_w,
		     uint32_t target_h, double blur, int blur_type,
		     const std::shared_ptr<WallpaperRequestResult> &result)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			const auto hit = cache_.find(key);
			if (hit != cache_.end()) {
				std::lock_guard<std::mutex> state_lock(result->mutex);
				result->image = hit->second;
				result->ready = true;
				return;
			}
			auto &waiting = pending_[key];
			waiting.push_back(result);
			if (waiting.size() > 1)
				return;
			jobs_.push_back(Job{key, path, target_w, target_h, blur, blur_type});
		}
		cv_.notify_one();
	}

private:
	struct Job {
		std::string key;
		std::string path;
		uint32_t target_w = 0;
		uint32_t target_h = 0;
		double blur = 0.0;
		int blur_type = 0;
	};

	WallpaperCache()
	{
		try {
			worker_ = std::thread(&WallpaperCache::loop, this);
		} catch (...) {
			blog(LOG_WARNING, "[TopScreen] wallpaper cache worker unavailable");
		}
	}

	~WallpaperCache()
	{
		if (!worker_.joinable())
			return;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stop_ = true;
		}
		cv_.notify_all();
		worker_.join();
	}

	void loop()
	{
		for (;;) {
			Job job;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				cv_.wait(lock, [this] { return stop_ || !jobs_.empty(); });
				if (stop_)
					return;
				job = std::move(jobs_.front());
				jobs_.pop_front();
				if (pending_.find(job.key) == pending_.end())
					continue;
			}

			core::Image image;
			try {
				image = compute_wallpaper(job.path, job.target_w, job.target_h,
							  job.blur, job.blur_type);
			} catch (...) {
				blog(LOG_WARNING, "[TopScreen] wallpaper cache job failed");
			}

			std::vector<std::shared_ptr<WallpaperRequestResult>> waiting;
			std::shared_ptr<const core::Image> stored;
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (!image.empty()) {
					stored = std::make_shared<const core::Image>(std::move(image));
					cache_[job.key] = stored;
					order_.push_back(job.key);
					while (order_.size() > 4) {
						cache_.erase(order_.front());
						order_.pop_front();
					}
				}
				const auto it = pending_.find(job.key);
				if (it != pending_.end()) {
					waiting = std::move(it->second);
					pending_.erase(it);
				}
			}

			for (const auto &result : waiting) {
				std::lock_guard<std::mutex> lock(result->mutex);
				result->ready = true;
				result->failed = !stored;
				if (stored)
					result->image = stored;
			}
		}
	}

	std::thread worker_;
	std::mutex mutex_;
	std::condition_variable cv_;
	std::deque<Job> jobs_;
	std::map<std::string, std::shared_ptr<const core::Image>> cache_;
	std::deque<std::string> order_;
	std::map<std::string, std::vector<std::shared_ptr<WallpaperRequestResult>>> pending_;
	bool stop_ = false;
};

} // namespace

WallpaperPipeline::~WallpaperPipeline()
{
	release();
}

std::string WallpaperPipeline::resolve_path(const char *raw)
{
	std::string path = raw ? raw : "";
	if (const char *builtin = wallpaper_path_for(path.c_str()))
		path = builtin;
	if (path.empty())
		return {};

	char *module_path = obs_module_file(path.c_str());
	if (!module_path)
		return path;
	std::string resolved = module_path;
	bfree(module_path);
	return resolved;
}

void WallpaperPipeline::release_texture()
{
	if (!texture_)
		return;
	obs_enter_graphics();
	gs_texture_destroy(texture_);
	obs_leave_graphics();
	texture_ = nullptr;
}

void WallpaperPipeline::release()
{
	release_texture();
	requested_.clear();
	resolved_.clear();
	pending_key_.clear();
	actual_key_.clear();
	failed_key_.clear();
	pending_.reset();
	aspect_ = 1.0f;
}

bool WallpaperPipeline::upload_texture(const core::Image &image)
{
	if (image.empty())
		return false;

	const uint8_t *data[1] = {image.pixels.data()};
	obs_enter_graphics();
	gs_texture_t *next = gs_texture_create(static_cast<uint32_t>(image.width),
					       static_cast<uint32_t>(image.height), GS_RGBA, 1, data, 0);
	obs_leave_graphics();
	if (!next)
		return false;

	if (texture_) {
		obs_enter_graphics();
		gs_texture_destroy(texture_);
		obs_leave_graphics();
	}
	texture_ = next;
	aspect_ = static_cast<float>(image.width) / static_cast<float>(image.height);
	return true;
}

bool WallpaperPipeline::poll_pending()
{
	if (!pending_)
		return false;

	std::shared_ptr<const core::Image> image;
	bool ready = false;
	bool failed = false;
	{
		std::lock_guard<std::mutex> lock(pending_->mutex);
		ready = pending_->ready;
		failed = pending_->failed;
		image = pending_->image;
	}
	if (!ready)
		return false;
	if (failed || !image || !upload_texture(*image)) {
		failed_key_ = pending_key_;
		pending_.reset();
		pending_key_.clear();
		return true;
	}

	actual_key_ = pending_key_;
	pending_.reset();
	pending_key_.clear();
	return true;
}

void WallpaperPipeline::update(const char *raw, uint32_t target_w, uint32_t target_h, double blur,
			       int blur_type)
{
	const char *requested = raw ? raw : "";
	if (requested_ != requested) {
		requested_ = requested;
		resolved_ = resolve_path(requested);
		/* Keep the previous texture visible while the new image is decoded and
		 * filtered. Releasing it here causes the solid-color flash on every
		 * wallpaper change. */
		pending_.reset();
		pending_key_.clear();
		failed_key_.clear();
	}

	if (resolved_.empty()) {
		release();
		return;
	}

	const std::string key = make_key(resolved_, target_w, target_h, blur, blur_type);
	if (actual_key_ == key || failed_key_ == key)
		return;

	/* A pending request may already be finished. Poll before deciding to wait,
	 * otherwise the ready texture is never uploaded and the frame stays solid
	 * until the background mode changes. */
	poll_pending();
	if (actual_key_ == key || failed_key_ == key)
		return;
	if (pending_ && pending_key_ == key)
		return;

	auto result = std::make_shared<WallpaperRequestResult>();
	pending_ = result;
	pending_key_ = key;
	WallpaperCache::instance().request(key, resolved_, target_w, target_h, blur, blur_type, result);
	poll_pending();
}

} // namespace topscreen
