#include "ts-click-sound.hpp"

#include <obs-module.h>

namespace topscreen {

bool ClickSound::load(const char *path)
{
	/* TODO(T3.6): decodificar o WAV e preparar o playback. */
	UNUSED_PARAMETER(path);
	loaded_ = false;
	return false;
}

void ClickSound::release()
{
	loaded_ = false;
	sound_ = nullptr;
}

void ClickSound::play()
{
	/* TODO(T3.6): disparar o som respeitando volume e limites. */
}

} // namespace topscreen
