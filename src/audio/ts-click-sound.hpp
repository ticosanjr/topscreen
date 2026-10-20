#pragma once

#include "core/wav.hpp"

#include <string>
#include <vector>

namespace topscreen {

/**
 * Click sounds (left/right button).
 *
 * Decodes the WAV (via core/wav) in load() and plays through the system
 * audio (PlaySoundW on Windows; paplay/pw-play/aplay on Linux). The OBS
 * channel playback comes with F6 (dedicated source).
 *
 * Playback never blocks the video thread: the child is spawned detached
 * (POSIX_SPAWN_SETSID) and reaped lazily on the next click. No global
 * signal handlers are installed.
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
		std::string module; // module-relative name used by the cache check
		std::string path;   // resolved absolute path
		core::WavData wav;
	};

	bool load_slot(Slot *slot, const char *module_file);
	void reap_children();

	bool loaded_ = false;
	Slot left_;
	Slot right_;
	std::string player_; // Linux: resolved player path
	bool player_checked_ = false;
	std::vector<int> children_; // Linux: player PIDs waiting to be reaped
};

} // namespace topscreen
