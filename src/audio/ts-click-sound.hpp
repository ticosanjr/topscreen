#pragma once

#include "core/wav.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace topscreen {

/**
 * Decoded click sounds (left/right button).
 *
 * Playback is mixed into the OBS audio stream by the
 * `topscreen_click_audio` source; this class only decodes/caches the WAV and
 * queues a voice on the process-wide ClickAudioBus.
 */
class ClickSound {
public:
	/** Loads the left/right files (module-relative). Unchanged slots are kept. */
	bool load(const char *left_module_file, const char *right_module_file);
	void release();
	/** Queues one click for the OBS audio source. */
	void play(bool right, double volume);
	bool loaded() const { return left_.loaded || right_.loaded; }
	/** Decoded left sound (empty when not loaded). For diagnostics/tests. */
	core::WavData wav() const;
	/** Resolved path of the loaded slot ("" when not loaded). Thread-safe copy. */
	std::string loaded_path(bool right) const;

private:
	struct Slot {
		bool loaded = false;
		std::string module; // module-relative name used by the cache check
		std::string path;   // resolved absolute path
		std::shared_ptr<const core::WavData> wav;
	};

	bool load_slot(Slot *slot, const char *module_file);

	mutable std::mutex mutex_;
	bool loaded_ = false;
	Slot left_;
	Slot right_;
};

} // namespace topscreen
