#pragma once

namespace topscreen {

/**
 * Som de clique.
 *
 * F3/T3.6. Nesta fase e apenas o esqueleto; em F6 o playback vai para o canal
 * de audio da fonte propria (obs_source_output_audio).
 */
class ClickSound {
public:
	bool load(const char *path);
	void release();
	void play();

	bool loaded() const { return loaded_; }

private:
	bool loaded_ = false;
	void *sound_ = nullptr;
};

} // namespace topscreen
