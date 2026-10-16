#pragma once

#include "core/wav.hpp"

namespace topscreen {

/**
 * Som de clique.
 *
 * Faz o decode do WAV (via core/wav) no load(). O playback entra em F6 (canal
 * de audio da fonte propria / obs_source_output_audio).
 */
class ClickSound {
public:
	/** Carrega um arquivo relativo ao diretorio de dados do modulo. */
	bool load(const char *module_file);
	void release();
	void play();

	bool loaded() const { return loaded_; }
	const core::WavData &wav() const { return wav_; }

private:
	bool loaded_ = false;
	core::WavData wav_;
};

} // namespace topscreen
