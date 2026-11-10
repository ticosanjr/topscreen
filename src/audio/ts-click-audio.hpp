#pragma once

#include "core/wav.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace topscreen {

/**
 * Process-wide click-sound mixer.
 *
 * The filter queues a decoded WAV when a mouse button is clicked; the
 * `topscreen_click_audio` OBS source drains the queue at the OBS audio rate
 * and outputs the mixed PCM through the normal source/mixer path.
 */
class ClickAudioBus {
public:
	static ClickAudioBus &instance();

	/** Queues one click. Safe to call from the video thread. */
	void push(std::shared_ptr<const core::WavData> wav, double volume);

	/** Mixes queued/active clicks into a stereo buffer (audio thread only). */
	void mix(float *left, float *right, uint32_t frames, uint32_t sample_rate);

	/** Drops pending/active clicks (scene switch, filter teardown). */
	void request_clear();

private:
	struct Voice {
		std::shared_ptr<const core::WavData> wav;
		double position = 0.0;
		double volume = 1.0;
	};

	ClickAudioBus() = default;

	std::mutex pending_mutex_;
	std::vector<Voice> pending_;
	std::vector<Voice> active_; // audio thread only
	std::atomic<bool> clear_requested_{false};
};

} // namespace topscreen
