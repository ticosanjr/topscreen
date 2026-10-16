#pragma once

#include "core/wav.hpp"

#include <string>

namespace topscreen {

/**
 * Som de clique.
 *
 * Faz o decode do WAV (via core/wav) no load() e toca via audio do sistema
 * (PlaySound no Windows; paplay/pw-play/aplay no Linux). O playback pelo canal
 * de audio do OBS entra em F6 (fonte propria).
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
	std::string path_;
	std::string player_; // Linux: caminho do player resolvido
	core::WavData wav_;
};

} // namespace topscreen
