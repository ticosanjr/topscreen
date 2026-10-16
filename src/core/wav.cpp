#include "wav.hpp"

#include <cstring>

namespace topscreen::core {

namespace {

uint16_t rd_u16(const uint8_t *p)
{
	return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

uint32_t rd_u32(const uint8_t *p)
{
	return static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24));
}

bool fourcc(const uint8_t *p, const char *tag)
{
	return std::memcmp(p, tag, 4) == 0;
}

float sample_to_float(const uint8_t *p, int bits, uint16_t format)
{
	if (format == 3) { /* IEEE float */
		float f = 0.0f;
		std::memcpy(&f, p, 4);
		return f;
	}

	switch (bits) {
	case 8:
		/* PCM 8 bits e sem sinal. */
		return (static_cast<int>(p[0]) - 128) / 128.0f;
	case 16: {
		const int16_t v = static_cast<int16_t>(rd_u16(p));
		return v / 32768.0f;
	}
	case 24: {
		int32_t v = p[0] | (p[1] << 8) | (p[2] << 16);
		if (v & 0x800000)
			v |= ~0xFFFFFF; /* extensao de sinal */
		return v / 8388608.0f;
	}
	case 32: {
		const int32_t v = static_cast<int32_t>(rd_u32(p));
		return static_cast<float>(static_cast<double>(v) / 2147483648.0);
	}
	default:
		return 0.0f;
	}
}

} // namespace

bool wav_parse(const uint8_t *data, size_t size, WavData *out, std::string *error)
{
	auto fail = [&](const char *msg) {
		if (error)
			*error = std::string("wav: ") + msg;
		return false;
	};

	if (!data || size < 12)
		return fail("arquivo muito curto");
	if (!fourcc(data, "RIFF") || !fourcc(data + 8, "WAVE"))
		return fail("nao e um RIFF/WAVE");

	uint16_t format = 0;
	uint16_t channels = 0;
	uint32_t sample_rate = 0;
	uint16_t bits = 0;
	const uint8_t *sample_data = nullptr;
	size_t sample_bytes = 0;

	size_t pos = 12;
	while (pos + 8 <= size) {
		const uint8_t *id = data + pos;
		const uint32_t chunk_size = rd_u32(data + pos + 4);
		const size_t body = pos + 8;
		if (body + chunk_size > size)
			break; /* chunk truncado */

		if (fourcc(id, "fmt ")) {
			if (chunk_size < 16)
				return fail("chunk fmt curto");
			format = rd_u16(data + body);
			channels = rd_u16(data + body + 2);
			sample_rate = rd_u32(data + body + 4);
			bits = rd_u16(data + body + 14);
		} else if (fourcc(id, "data")) {
			sample_data = data + body;
			sample_bytes = chunk_size;
		}

		pos = body + chunk_size + (chunk_size & 1u); /* padding par */
	}

	if (format != 1 && format != 3)
		return fail("formato nao suportado (apenas PCM/float)");
	if (channels == 0)
		return fail("sem canais");
	if (sample_rate == 0)
		return fail("sample rate invalido");
	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		return fail("bits por amostra nao suportado");
	if (format == 3 && bits != 32)
		return fail("float exige 32 bits");
	if (!sample_data || sample_bytes == 0)
		return fail("sem chunk data");

	const size_t bytes_per_sample = bits / 8u;
	const size_t total_samples = sample_bytes / bytes_per_sample;

	out->sample_rate = static_cast<int>(sample_rate);
	out->channels = static_cast<int>(channels);
	out->bits_per_sample = static_cast<int>(bits);
	out->samples.clear();
	out->samples.reserve(total_samples);

	for (size_t i = 0; i < total_samples; ++i)
		out->samples.push_back(sample_to_float(sample_data + i * bytes_per_sample, bits, format));

	return true;
}

} // namespace topscreen::core
