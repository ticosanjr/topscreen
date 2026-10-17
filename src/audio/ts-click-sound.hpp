#pragma once

#include "core/wav.hpp"

#include <string>

namespace topscreen {

/**
 * Click sounds (left/right button).
 *
 * Decodes the WAV (via core/wav) in load() and plays through the system
 * audio (PlaySound on Windows; paplay/pw-play/aplay on Linux). The OBS
 * channel playback comes with F6 (dedicated source).
 *
 * Playback never blocks the video thread: the child is spawned detached
 * (SIGCHLD ignored) and never waited on.
 */
class ClickSound {
public:
	/** Loads the left/right files (module-relative). Unchanged slots are kept. */
	bool load(const char *left_module_file, const char *right_module_file);
	void release();
	/** Plays a side at the given volume (0..1; only paplay honors it). */
	void play(bool right, double volume);
	bool loaded() const { return left_.loaded || right_.loaded; }
	const core::WavData &wav() const { return left_.wav; }
	/** Resolved path of the loaded slot ("" when not loaded). */
	const std::string &loaded_path(bool right) const { return right ? right_.path : left_.path; }

private:
	struct Slot {
		bool loaded = false;
		std::string path;
		core::WavData wav;
	};

	bool load_slot(Slot *slot, const char *module_file);

	bool loaded_ = false;
	Slot left_;
	Slot right_;
	std::string player_; // Linux: resolved player path
};

} // namespace topscreen
