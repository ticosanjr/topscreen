#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Samples of a decoded WAV (interleaved float, -1..1). */
struct WavData {
	int sample_rate = 0;
	int channels = 0;
	int bits_per_sample = 0;
	std::vector<float> samples;

	size_t frame_count() const { return channels > 0 ? samples.size() / static_cast<size_t>(channels) : 0; }
	bool empty() const { return samples.empty(); }
};

/**
 * Decodes a WAV (RIFF/WAVE) in memory.
 *
 * Supports PCM 8/16/24/32 bits and 32-bit float, mono or multichannel.
 * Returns false and fills `error` on an invalid/unsupported format.
 */
bool wav_parse(const uint8_t *data, size_t size, WavData *out, std::string *error);

} // namespace topscreen::core
