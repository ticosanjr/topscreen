#pragma once

#include "core/wav.hpp"

#include <string>

namespace topscreen {

/**
 * Click sound.
 *
 * Decodes the WAV (via core/wav) in load() and plays it through the system audio
 * (PlaySound on Windows; paplay/pw-play/aplay on Linux). Playback through the OBS
 * audio channel is planned for F6 (own source).
 */
class ClickSound {
public:
	/** Loads a file relative to the module's data directory. */
	bool load(const char *module_file);
	void release();
	void play();

	bool loaded() const { return loaded_; }
	const core::WavData &wav() const { return wav_; }

private:
	bool loaded_ = false;
	std::string path_;
	std::string player_; // Linux: resolved player path
	core::WavData wav_;
};

} // namespace topscreen
