#include "ts-click-sound.hpp"

#include <obs-module.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace topscreen {

bool ClickSound::load(const char *module_file)
{
	release();

	if (!module_file || !*module_file)
		return false;

	char *path = obs_module_file(module_file);
	if (!path) {
		blog(LOG_WARNING, "[TopScreen] som nao encontrado: %s", module_file);
		return false;
	}

	std::ifstream in(path, std::ios::binary);
	bfree(path);
	if (!in)
		return false;

	const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
					 std::istreambuf_iterator<char>());

	std::string err;
	core::WavData wav;
	if (!core::wav_parse(bytes.data(), bytes.size(), &wav, &err)) {
		blog(LOG_WARNING, "[TopScreen] WAV invalido (%s): %s", module_file, err.c_str());
		return false;
	}

	wav_ = std::move(wav);
	loaded_ = true;

	blog(LOG_INFO, "[TopScreen] som carregado: %s (%d Hz, %d canais)", module_file, wav_.sample_rate,
	     wav_.channels);
	return true;
}

void ClickSound::release()
{
	loaded_ = false;
	wav_ = core::WavData{};
}

void ClickSound::play()
{
	/* TODO(F6.5): tocar no canal de audio da fonte (obs_source_output_audio). */
}

} // namespace topscreen
