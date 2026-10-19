#include "monitor.hpp"

#include <cmath>

namespace topscreen::core {

namespace {

bool same_size(const Monitor &m, uint32_t w, uint32_t h)
{
	if (w == 0 || h == 0)
		return false;
	return std::fabs(m.width - static_cast<double>(w)) < 1.0 &&
	       std::fabs(m.height - static_cast<double>(h)) < 1.0;
}

} // namespace

bool pick_monitor(const Monitor *monitors, int count, int hint, bool trust_hint,
		  uint32_t capture_w, uint32_t capture_h, Monitor *out)
{
	if (!monitors || count <= 0 || !out)
		return false;
	/* No capture size yet: nothing sensible to decide. */
	if (capture_w == 0 || capture_h == 0)
		return false;

	const bool hint_valid = hint >= 0 && hint < count;

	/* A hint that also matches the size is the best evidence. */
	if (hint_valid && same_size(monitors[hint], capture_w, capture_h)) {
		*out = monitors[hint];
		return true;
	}

	/* Otherwise a single monitor with the exact capture size is unambiguous
	 * (this is what fixes stacked side-by-side/above-below setups). */
	int match = -1;
	int matches = 0;
	for (int i = 0; i < count; ++i) {
		if (same_size(monitors[i], capture_w, capture_h)) {
			match = i;
			++matches;
		}
	}
	if (matches == 1) {
		*out = monitors[match];
		return true;
	}

	/* Trusted index (the platform enumerates the same way the capture does):
	 * use it even if the reported size does not match exactly. */
	if (trust_hint && hint_valid) {
		*out = monitors[hint];
		return true;
	}

	/* Several identical monitors and no trustworthy index: undecidable. */
	return false;
}

} // namespace topscreen::core
