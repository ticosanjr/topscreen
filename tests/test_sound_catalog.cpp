#include "core/sound_catalog.hpp"
#include "core/wav.hpp"
#include "test_framework.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

using namespace topscreen::core;

namespace {

std::vector<uint8_t> read_bytes(const std::string &path)
{
	std::ifstream in(path, std::ios::binary);
	return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),
				    std::istreambuf_iterator<char>());
}

std::string lower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

} // namespace

TEST(sound_catalog_has_all_bundled_files)
{
	CHECK_EQ(kSoundCount, 26);

	std::set<std::string> labels;
	std::set<std::string> paths;
	for (int i = 0; i < kSoundCount; ++i) {
		const SoundEntry &s = kSounds[i];
		REQUIRE(s.label != nullptr);
		REQUIRE(s.path != nullptr);

		++tst::checks();
		if (!labels.insert(s.label).second)
			tst::fail(__FILE__, __LINE__,
				  std::string("duplicate sound label: ") + s.label);
		++tst::checks();
		if (!paths.insert(s.path).second)
			tst::fail(__FILE__, __LINE__,
				  std::string("duplicate sound path: ") + s.path);

		const std::string label = lower(s.label);
		++tst::checks();
		if (label.find("mixkit") != std::string::npos ||
		    label.find("starter") != std::string::npos ||
		    label.find("pointer") != std::string::npos)
			tst::fail(__FILE__, __LINE__,
				  std::string("forbidden word in sound label: ") + s.label);

		const std::string path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/" + s.path;
		const auto bytes = read_bytes(path);
		REQUIRE(!bytes.empty());

		WavData wav;
		std::string err;
		++tst::checks();
		if (!wav_parse(bytes.data(), bytes.size(), &wav, &err))
			tst::fail(__FILE__, __LINE__,
				  std::string("invalid WAV: ") + path + " -> " + err);
		CHECK_EQ(wav.sample_rate, 44100);
		CHECK_EQ(wav.channels, 1);
		CHECK(wav.frame_count() > 100);
	}
}
