#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace topscreen::core {

struct KeyOverlayConfig {
	bool enabled = true;
	bool modifiers_only = false;
	double ttl_ms = 2000.0;
	int max_keys = 6;
	double pos_x = 0.5;  // 0..1 (horizontal center of the band)
	double pos_y = 0.93; // 0..1 (text baseline)
	double size = 1.0;   // text size multiplier
	std::string font = "fonts/DejaVuSans-Bold.ttf";
	uint32_t color = 0xFFFFFFFF; // text color (0xAABBGGRR)
	bool shadow = true;          // text outline/shadow
	bool band = true;            // band behind the text
	uint32_t band_color = 0x99000000;
};

struct KeyRender {
	std::string label;
	double progress = 0.0; // 0..1
	bool modifier = false;
};

/** Formats a combo: {"Ctrl","Shift"} + "K" => "Ctrl + Shift + K". */
std::string format_combo(const std::vector<std::string> &modifiers, const std::string &key);

/**
 * Key overlay.
 *
 * Keeps a queue of recent keys (with TTL), allowing it to show what is
 * being typed. It does not capture keys (that is platform-specific); it only
 * models the displayed state.
 */
class KeyOverlayModel {
public:
	void configure(const KeyOverlayConfig &cfg);

	/** Registers a key. Repeating the same key resets the timer. */
	void press(const std::string &label, bool is_modifier, double now_ms);

	void clear();

	/** Removes expired keys. */
	void tick(double now_ms);

	int active_count() const { return static_cast<int>(entries_.size()); }

	/** Fills up to `max` entries (from oldest to newest). */
	int collect(double now_ms, KeyRender *out, int max) const;

private:
	struct Entry {
		std::string label;
		double born_ms = 0.0;
		bool modifier = false;
	};

	std::vector<Entry> entries_;
	KeyOverlayConfig cfg_;
};

} // namespace topscreen::core
