#include "key_overlay.hpp"

#include "math.hpp"

namespace topscreen::core {

std::string format_combo(const std::vector<std::string> &modifiers, const std::string &key)
{
	std::string out;
	for (const auto &m : modifiers) {
		if (m.empty())
			continue;
		if (!out.empty())
			out += " + ";
		out += m;
	}
	if (!key.empty()) {
		if (!out.empty())
			out += " + ";
		out += key;
	}
	return out;
}

bool is_special_key_label(std::string_view label)
{
	static constexpr std::string_view kSpecial[] = {
		"Backspace", "Enter", "Tab", "Esc", "Del", "Delete",
		"Left",      "Right", "Up",  "Down",
	};
	for (std::string_view s : kSpecial) {
		if (label == s)
			return true;
	}
	return false;
}

void KeyOverlayModel::configure(const KeyOverlayConfig &cfg)
{
	cfg_ = cfg;
	if (cfg_.max_keys < 1)
		cfg_.max_keys = 1;
	if (cfg_.ttl_ms <= 0.0)
		cfg_.ttl_ms = 1.0;
	/* The queue never holds more than max_keys: reserve once instead of
	 * growing the vector on the first keystrokes. Keep the hint bounded so
	 * a malformed config cannot request a huge allocation here. */
	entries_.reserve(static_cast<size_t>(std::min(cfg_.max_keys, 64)));
}

void KeyOverlayModel::press(std::string_view label, bool is_modifier, double now_ms,
			    uint32_t modifiers)
{
	if (!cfg_.enabled || label.empty())
		return;
	if (cfg_.modifiers_only) {
		if (!is_modifier)
			return;
	} else if (!is_modifier && !cfg_.show_special && is_special_key_label(label)) {
		return;
	}

	/* A space is a literal blank, not a named key or icon. */
	const std::string_view shown = label == "Space" ? std::string_view(" ") : label;

	/* With shortcuts on, modifiers are folded into the non-modifier key that
	 * follows them. Modifier-only mode still shows each modifier by name. */
	if (cfg_.show_shortcuts && !cfg_.modifiers_only && !is_modifier && modifiers) {
		char combo[KeyLabel::kCapacity];
		size_t used = 0;
		auto append = [&](std::string_view part) {
			if (part.empty())
				return;
			if (used && used < KeyLabel::kCapacity - 1) {
				const char sep[] = " + ";
				const size_t sn = std::min(sizeof(sep) - 1, KeyLabel::kCapacity - 1 - used);
				std::memcpy(combo + used, sep, sn);
				used += sn;
			}
			const size_t n = std::min(part.size(), KeyLabel::kCapacity - 1 - used);
			if (n) {
				std::memcpy(combo + used, part.data(), n);
				used += n;
			}
		};
		if (modifiers & KeyModCtrl)
			append("Ctrl");
		if (modifiers & KeyModShift)
			append("Shift");
		if (modifiers & KeyModAlt)
			append("Alt");
		if (modifiers & KeyModWin)
			append("Win");
		append(shown);
		Entry entry;
		entry.label.assign(std::string_view(combo, used));
		entry.born_ms = now_ms;
		entry.modifier = false;
		entries_.push_back(entry);
	} else if (cfg_.show_shortcuts && !cfg_.modifiers_only && is_modifier) {
		/* Shortcut mode: a standalone modifier is never a key entry. */
		return;
	} else {
		/* A repeated modifier is one physical key held down, not two labels.
		 * Refresh the existing entry instead of stacking a duplicate. */
		if (is_modifier && !entries_.empty() && entries_.back().modifier &&
		    entries_.back().label.view() == shown) {
			entries_.back().born_ms = now_ms;
			return;
		}
		/* Every other keystroke is appended as its own entry (repeats
		 * included), so what you typed is what you see. */
		Entry entry;
		entry.label.assign(shown);
		entry.born_ms = now_ms;
		entry.modifier = is_modifier;
		entries_.push_back(entry);
	}

	/* Respects the limit, discarding the oldest ones. */
	while (static_cast<int>(entries_.size()) > cfg_.max_keys)
		entries_.erase(entries_.begin());
}

void KeyOverlayModel::clear()
{
	entries_.clear();
}

void KeyOverlayModel::tick(double now_ms)
{
	for (auto it = entries_.begin(); it != entries_.end();) {
		if ((now_ms - it->born_ms) >= cfg_.ttl_ms)
			it = entries_.erase(it);
		else
			++it;
	}
}

int KeyOverlayModel::collect(double now_ms, KeyRender *out, int max) const
{
	int written = 0;
	for (const auto &e : entries_) {
		if (written >= max)
			break;
		KeyRender &r = out[written++];
		r.label = e.label;
		r.modifier = e.modifier;
		r.progress = clamp01((now_ms - e.born_ms) / cfg_.ttl_ms);
	}
	return written;
}

} // namespace topscreen::core
