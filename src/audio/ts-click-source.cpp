#include "ts-click-source.hpp"

#include "ts-click-audio.hpp"

#include <obs-frontend-api.h>
#include <util/platform.h>

#include <media-io/audio-io.h>

#include <cstring>

namespace topscreen {

namespace {

constexpr const char *kSourceId = "topscreen_click_audio";
constexpr const char *kSourceName = "TopScreen Click Sound";
constexpr uint32_t kDefaultMixers = 0x3F; // OBS mixer 1..6

struct ClickAudioSource {
	uint64_t next_ts = 0;
};

const char *click_audio_name(void *)
{
	return obs_module_text("ClickAudio.Name");
}

void *click_audio_create(obs_data_t *, obs_source_t *)
{
	return new ClickAudioSource();
}

void click_audio_destroy(void *data)
{
	delete static_cast<ClickAudioSource *>(data);
}

void click_audio_activate(void *data)
{
	if (auto *state = static_cast<ClickAudioSource *>(data))
		state->next_ts = 0;
}

bool click_audio_render(void *data, uint64_t *ts_out, struct obs_source_audio_mix *audio,
			uint32_t mixers, size_t channels, size_t sample_rate)
{
	auto *state = static_cast<ClickAudioSource *>(data);
	if (!state || !ts_out || !audio || sample_rate == 0)
		return false;

	float left[AUDIO_OUTPUT_FRAMES] = {};
	float right[AUDIO_OUTPUT_FRAMES] = {};
	ClickAudioBus::instance().mix(left, right, AUDIO_OUTPUT_FRAMES,
				      static_cast<uint32_t>(sample_rate));

	for (size_t mix = 0; mix < MAX_AUDIO_MIXES; ++mix) {
		if ((mixers & (1u << mix)) == 0)
			continue;

		float *out0 = audio->output[mix].data[0];
		if (!out0)
			continue;

		if (channels == 1) {
			for (uint32_t i = 0; i < AUDIO_OUTPUT_FRAMES; ++i)
				out0[i] = (left[i] + right[i]) * 0.5f;
		} else {
			for (uint32_t i = 0; i < AUDIO_OUTPUT_FRAMES; ++i)
				out0[i] = left[i];
			if (float *out1 = audio->output[mix].data[1]) {
				for (uint32_t i = 0; i < AUDIO_OUTPUT_FRAMES; ++i)
					out1[i] = right[i];
			}
			for (size_t ch = 2; ch < channels; ++ch) {
				if (float *out = audio->output[mix].data[ch])
					std::memset(out, 0, sizeof(float) * AUDIO_OUTPUT_FRAMES);
			}
		}
	}

	if (state->next_ts == 0)
		state->next_ts = os_gettime_ns();
	*ts_out = state->next_ts;
	state->next_ts += static_cast<uint64_t>(AUDIO_OUTPUT_FRAMES) * 1000000000ULL /
			  static_cast<uint64_t>(sample_rate);
	return true;
}

obs_properties_t *click_audio_properties(void *)
{
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_text(props, "click_audio_hint", obs_module_text("ClickAudio.Hint"),
				OBS_TEXT_INFO);
	return props;
}

} // namespace

struct obs_source_info ts_click_audio_source_info = {
	.id = kSourceId,
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = click_audio_name,
	.create = click_audio_create,
	.destroy = click_audio_destroy,
	.get_properties = click_audio_properties,
	.activate = click_audio_activate,
	.audio_render = click_audio_render,
};

bool ts_click_audio_add_to_current_scene()
{
	obs_source_t *scene_source = obs_frontend_get_current_scene();
	if (!scene_source)
		return false;

	obs_scene_t *scene = obs_scene_from_source(scene_source);
	if (!scene) {
		obs_source_release(scene_source);
		return false;
	}

	struct FindState {
		const char *id = nullptr;
		bool found = false;
	};
	FindState find{kSourceId, false};
	obs_scene_enum_items(
		scene,
		[](obs_scene_t *, obs_sceneitem_t *item, void *param) {
			auto *state = static_cast<FindState *>(param);
			obs_source_t *source = obs_sceneitem_get_source(item);
			if (source && state->id &&
			    std::strcmp(obs_source_get_id(source), state->id) == 0) {
				state->found = true;
				return false;
			}
			return true;
		},
		&find);
	if (find.found) {
		obs_source_release(scene_source);
		return true;
	}

	obs_source_t *source = obs_source_create(kSourceId, kSourceName, nullptr, nullptr);
	bool ok = false;
	if (source) {
		obs_source_set_audio_mixers(source, kDefaultMixers);
		ok = obs_scene_add(scene, source) != nullptr;
		obs_source_release(source);
	}

	obs_source_release(scene_source);
	if (ok)
		blog(LOG_INFO, "[TopScreen] click sound source added to the current scene");
	else
		blog(LOG_WARNING, "[TopScreen] could not add the click sound source");
	return ok;
}

} // namespace topscreen
