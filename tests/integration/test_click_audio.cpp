#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <media-io/audio-io.h>

#include <cmath>
#include <vector>

#include "audio/ts-click-audio.hpp"
#include "audio/ts-click-source.hpp"
#include "audio/ts-click-sound.hpp"

using namespace topscreen;

TEST(click_audio_source_mixes_queued_clicks)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	ClickSound sound;
	REQUIRE(sound.load("sounds/click-soft.wav", "sounds/click-mouse-right.wav"));
	ClickAudioBus::instance().request_clear();

	void *source = ts_click_audio_source_info.create(nullptr, nullptr);
	REQUIRE(source != nullptr);

	std::vector<float> left(AUDIO_OUTPUT_FRAMES, 0.0f);
	std::vector<float> right(AUDIO_OUTPUT_FRAMES, 0.0f);
	obs_source_audio_mix mix = {};
	mix.output[0].data[0] = left.data();
	mix.output[0].data[1] = right.data();

	uint64_t ts = 0;
	CHECK(ts_click_audio_source_info.audio_render(source, &ts, &mix, 1, 2, 48000));
	CHECK(ts > 0);
	/* Silence when no click is queued. */
	double idle = 0.0;
	for (uint32_t i = 0; i < AUDIO_OUTPUT_FRAMES; ++i)
		idle += std::abs(left[i]) + std::abs(right[i]);
	CHECK(idle == 0.0);

	sound.play(false, 1.0);
	CHECK(ts_click_audio_source_info.audio_render(source, &ts, &mix, 1, 2, 48000));
	double energy = 0.0;
	for (uint32_t i = 0; i < AUDIO_OUTPUT_FRAMES; ++i)
		energy += std::abs(left[i]) + std::abs(right[i]);
	CHECK(energy > 0.0);

	ts_click_audio_source_info.destroy(source);
	sound.release();
	ClickAudioBus::instance().request_clear();
}
