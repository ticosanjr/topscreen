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

void KeyOverlayModel::press(std::string_view label, bool is_modifier, double now_ms)
{
	if (!cfg_.enabled || label.empty())
		return;
	if (cfg_.modifiers_only && !is_modifier)
		return;

	/* Every keystroke is appended as its own entry (repeats included), so
	 * what you typed is what you see. Each letter expires on its own TTL. */
	Entry entry;
	entry.label.assign(label);
	entry.born_ms = now_ms;
	entry.modifier = is_modifier;
	entries_.push_back(entry);

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
