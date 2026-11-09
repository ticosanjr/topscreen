#include "ts-input.hpp"

/*
 * No native backend: input unavailable (graceful degradation - Tier 4).
 * Empty when a real platform backend is compiled.
 */

#if !defined(_WIN32) && !defined(__linux__)
#include <cstdlib>

#include <string>

namespace topscreen {

void *ts_input_platform_start(obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	return reinterpret_cast<void *>(1);
}

void ts_input_platform_stop(void *state)
{
	UNUSED_PARAMETER(state);
}

void ts_input_platform_poll(void *state, InputState *out)
{
	UNUSED_PARAMETER(state);
	out->available = false;
	out->tier = 0;
	out->key_count = 0;

	/*
	 * Debug hook: with TOPSCREEN_DEBUG_KEY defined, injects that key
	 * periodically so the overlay can be tested without an input back-end.
	 */
	static int counter = 0;
	const char *debug = std::getenv("TOPSCREEN_DEBUG_KEY");
	if (debug && *debug && (counter++ % 60) == 0) {
		out->key_labels[0] = debug;
		out->key_modifier[0] = false;
		out->key_count = 1;
	}
}

} // namespace topscreen

#endif
