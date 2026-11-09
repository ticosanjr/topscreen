#pragma once

#include <obs-module.h>

#include <atomic>
#include <cstdint>

namespace topscreen::core {
struct Config;
}

namespace topscreen {

/**
 * Origin of the captured monitor in global desktop coordinates.
 *
 * The input reports global positions, so a capture of a monitor below/left of
 * the primary one needs this offset before normalizing. The origin is resolved
 * from the parent capture's settings, refreshed every ~10 s (layout changes)
 * and only on the video thread.
 */
class MonitorOrigin {
public:
	void tick(obs_source_t *filter, const core::Config &cfg, uint32_t target_w,
		  uint32_t target_h);

	bool known() const { return known_.load(); }
	double x() const { return known_.load() ? x_.load() : 0.0; }
	double y() const { return known_.load() ? y_.load() : 0.0; }

private:
	void refresh(obs_source_t *filter, const core::Config &cfg, uint32_t target_w,
		     uint32_t target_h);

	std::atomic<double> x_{0.0};
	std::atomic<double> y_{0.0};
	std::atomic<bool> known_{false};
	uint32_t target_w_ = 0;
	uint32_t target_h_ = 0;
	int age_ticks_ = 0;
};

} // namespace topscreen
