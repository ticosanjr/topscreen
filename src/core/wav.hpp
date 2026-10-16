#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

/** Amostras de um WAV decodificado (float intercalado, -1..1). */
struct WavData {
	int sample_rate = 0;
	int channels = 0;
	int bits_per_sample = 0;
	std::vector<float> samples;

	size_t frame_count() const { return channels > 0 ? samples.size() / static_cast<size_t>(channels) : 0; }
	bool empty() const { return samples.empty(); }
};

/**
 * Decodifica um WAV (RIFF/WAVE) em memoria.
 *
 * Suporta PCM 8/16/24/32 bits e float 32 bits, mono ou multicanal.
 * Retorna false e preenche `error` em caso de formato invalido/nao suportado.
 */
bool wav_parse(const uint8_t *data, size_t size, WavData *out, std::string *error);

} // namespace topscreen::core
