#include "core/wav.hpp"
#include "test_framework.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace topscreen::core;

namespace {

void put_u32(std::vector<uint8_t> &v, uint32_t x)
{
	v.push_back(static_cast<uint8_t>(x & 0xFF));
	v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
	v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
	v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
}

void put_u16(std::vector<uint8_t> &v, uint16_t x)
{
	v.push_back(static_cast<uint8_t>(x & 0xFF));
	v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
}

void put_tag(std::vector<uint8_t> &v, const char *t)
{
	for (int i = 0; i < 4; ++i)
		v.push_back(static_cast<uint8_t>(t[i]));
}

std::vector<uint8_t> make_wav(uint16_t format, uint16_t channels, uint32_t rate, uint16_t bits,
			      const std::vector<uint8_t> &data)
{
	std::vector<uint8_t> v;
	put_tag(v, "RIFF");
	put_u32(v, static_cast<uint32_t>(36 + data.size()));
	put_tag(v, "WAVE");
	put_tag(v, "fmt ");
	put_u32(v, 16);
	put_u16(v, format);
	put_u16(v, channels);
	put_u32(v, rate);
	put_u32(v, rate * channels * (bits / 8));
	put_u16(v, static_cast<uint16_t>(channels * (bits / 8)));
	put_u16(v, bits);
	put_tag(v, "data");
	put_u32(v, static_cast<uint32_t>(data.size()));
	v.insert(v.end(), data.begin(), data.end());
	return v;
}

std::vector<uint8_t> le16(int16_t s)
{
	return {static_cast<uint8_t>(s & 0xFF), static_cast<uint8_t>((s >> 8) & 0xFF)};
}

} // namespace

TEST(wav_pcm16_mono)
{
	std::vector<uint8_t> data;
	auto s0 = le16(0);
	auto s1 = le16(16384); /* 0.5 */
	data.insert(data.end(), s0.begin(), s0.end());
	data.insert(data.end(), s1.begin(), s1.end());

	const auto bytes = make_wav(1, 1, 44100, 16, data);
	WavData w;
	std::string err;
	REQUIRE(wav_parse(bytes.data(), bytes.size(), &w, &err));

	CHECK_EQ(w.sample_rate, 44100);
	CHECK_EQ(w.channels, 1);
	CHECK_EQ(w.bits_per_sample, 16);
	CHECK_EQ(static_cast<int>(w.samples.size()), 2);
	CHECK_EQ(static_cast<int>(w.frame_count()), 2);
	CHECK_NEAR(w.samples[0], 0.0, 1e-6);
	CHECK_NEAR(w.samples[1], 0.5, 1e-4);
}

TEST(wav_pcm16_stereo_frame_count)
{
	std::vector<uint8_t> data;
	for (int i = 0; i < 8; ++i) { /* 4 frames x 2 channels */
		auto s = le16(1000);
		data.insert(data.end(), s.begin(), s.end());
	}
	const auto bytes = make_wav(1, 2, 48000, 16, data);
	WavData w;
	std::string err;
	REQUIRE(wav_parse(bytes.data(), bytes.size(), &w, &err));
	CHECK_EQ(w.channels, 2);
	CHECK_EQ(static_cast<int>(w.samples.size()), 8);
	CHECK_EQ(static_cast<int>(w.frame_count()), 4);
}

TEST(wav_pcm8_unsigned)
{
	const std::vector<uint8_t> data = {128, 255}; /* 0.0, ~0.992 */
	const auto bytes = make_wav(1, 1, 8000, 8, data);
	WavData w;
	std::string err;
	REQUIRE(wav_parse(bytes.data(), bytes.size(), &w, &err));
	CHECK_NEAR(w.samples[0], 0.0, 1e-6);
	CHECK_NEAR(w.samples[1], 127.0 / 128.0, 1e-6);
}

TEST(wav_pcm24_sign_extension)
{
	/* +0.5 => 0x400000 (little-endian 00 00 40) */
	const std::vector<uint8_t> pos = {0x00, 0x00, 0x40};
	const auto bytes = make_wav(1, 1, 44100, 24, pos);
	WavData w;
	std::string err;
	REQUIRE(wav_parse(bytes.data(), bytes.size(), &w, &err));
	CHECK_NEAR(w.samples[0], 0.5, 1e-6);
}

TEST(wav_float32)
{
	const float f = 0.25f;
	std::vector<uint8_t> data(4);
	std::memcpy(data.data(), &f, 4);
	const auto bytes = make_wav(3, 1, 44100, 32, data);
	WavData w;
	std::string err;
	REQUIRE(wav_parse(bytes.data(), bytes.size(), &w, &err));
	CHECK_NEAR(w.samples[0], 0.25, 1e-6);
}

TEST(wav_errors)
{
	WavData w;
	std::string err;

	const std::vector<uint8_t> tiny = {1, 2, 3};
	CHECK(!wav_parse(tiny.data(), tiny.size(), &w, &err));
	CHECK(!err.empty());

	std::vector<uint8_t> bad_magic = make_wav(1, 1, 44100, 16, {0, 0});
	bad_magic[0] = 'X';
	CHECK(!wav_parse(bad_magic.data(), bad_magic.size(), &w, &err));

	/* unsupported format (2 = ADPCM) */
	const auto adpcm = make_wav(2, 1, 44100, 16, {0, 0});
	CHECK(!wav_parse(adpcm.data(), adpcm.size(), &w, &err));

	CHECK(!wav_parse(nullptr, 0, &w, &err));
}

TEST(wav_real_files)
{
#ifdef TOPSCREEN_TEST_DATA_DIR
	const char *files[] = {"click-soft.wav", "click-hard.wav"};
	for (const char *name : files) {
		const std::string path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/sounds/" + name;
		std::ifstream in(path, std::ios::binary);
		REQUIRE(in.good());
		const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
						 std::istreambuf_iterator<char>());

		WavData w;
		std::string err;
		++tst::checks();
		if (!wav_parse(bytes.data(), bytes.size(), &w, &err))
			tst::fail(__FILE__, __LINE__, "invalid WAV: " + path + " -> " + err);

		CHECK(!w.empty());
		CHECK_EQ(w.sample_rate, 44100);
		CHECK_EQ(w.channels, 1);
		CHECK(w.frame_count() > 1000);
	}
#endif
}
