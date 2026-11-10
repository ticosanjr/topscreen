#include "ts-click-audio.hpp"

#include <algorithm>
#include <cmath>

namespace topscreen {

ClickAudioBus &ClickAudioBus::instance()
{
	static ClickAudioBus bus;
	return bus;
}

void ClickAudioBus::push(std::shared_ptr<const core::WavData> wav, double volume)
{
	if (!wav || wav->empty() || wav->channels <= 0 || wav->sample_rate <= 0)
		return;

	volume = std::clamp(volume, 0.0, 1.0);
	std::lock_guard<std::mutex> lock(pending_mutex_);
	/* A missing audio source must not grow the queue forever. */
	if (pending_.size() >= 32)
		pending_.erase(pending_.begin());
	pending_.push_back(Voice{std::move(wav), 0.0, volume});
}

void ClickAudioBus::mix(float *left, float *right, uint32_t frames, uint32_t sample_rate)
{
	if ((!left && !right) || frames == 0 || sample_rate == 0)
		return;

	if (clear_requested_.exchange(false))
		active_.clear();

	{
		std::lock_guard<std::mutex> lock(pending_mutex_);
		if (!pending_.empty()) {
			active_.reserve(active_.size() + pending_.size());
			for (auto &voice : pending_)
				active_.push_back(std::move(voice));
			pending_.clear();
		}
	}

	for (auto it = active_.begin(); it != active_.end();) {
		Voice &voice = *it;
		const core::WavData &wav = *voice.wav;
		const size_t source_frames = wav.frame_count();
		if (source_frames == 0) {
			it = active_.erase(it);
			continue;
		}

		const double step = static_cast<double>(wav.sample_rate) / sample_rate;
		for (uint32_t i = 0; i < frames && voice.position < source_frames; ++i) {
			const size_t frame0 = static_cast<size_t>(voice.position);
			const size_t frame1 = std::min(frame0 + 1, source_frames - 1);
			const double frac = voice.position - static_cast<double>(frame0);

			auto sample_at = [&](size_t frame, int channel) -> float {
				const size_t index0 =
					frame * static_cast<size_t>(wav.channels) +
					static_cast<size_t>(channel);
				const size_t index1 =
					frame1 * static_cast<size_t>(wav.channels) +
					static_cast<size_t>(channel);
				const float a = wav.samples[index0];
				const float b = wav.samples[index1];
				return static_cast<float>(a + (b - a) * frac);
			};

			float l = 0.0f;
			float r = 0.0f;
			if (wav.channels == 1) {
				l = r = sample_at(frame0, 0);
			} else {
				l = sample_at(frame0, 0);
				r = sample_at(frame0, 1);
			}
			l *= static_cast<float>(voice.volume);
			r *= static_cast<float>(voice.volume);

			if (left && right) {
				left[i] += l;
				right[i] += r;
			} else if (left) {
				left[i] += (l + r) * 0.5f;
			} else {
				right[i] += (l + r) * 0.5f;
			}

			voice.position += step;
		}

		if (voice.position >= source_frames)
			it = active_.erase(it);
		else
			++it;
	}
}

void ClickAudioBus::request_clear()
{
	clear_requested_.store(true);
	std::lock_guard<std::mutex> lock(pending_mutex_);
	pending_.clear();
}

} // namespace topscreen
