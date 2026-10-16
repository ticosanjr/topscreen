#include "click_fx.hpp"

namespace topscreen::core {

void ClickFxModel::spawn(double x, double y, double now_ms, bool right)
{
	/* Procura um slot livre; se nao houver, recicla o mais antigo. */
	int index = -1;
	for (int i = 0; i < kMaxSlots; ++i) {
		const int idx = (next_ + i) % kMaxSlots;
		if (!slots_[idx].used) {
			index = idx;
			break;
		}
	}

	if (index < 0) {
		index = next_;
		for (int i = 0; i < kMaxSlots; ++i) {
			if (slots_[i].born_ms < slots_[index].born_ms)
				index = i;
		}
	}

	Slot &s = slots_[index];
	s.used = true;
	s.x = x;
	s.y = y;
	s.born_ms = now_ms;
	s.cfg = right ? right_ : left_;

	next_ = (index + 1) % kMaxSlots;
}

void ClickFxModel::tick(double now_ms)
{
	for (int i = 0; i < kMaxSlots; ++i) {
		Slot &s = slots_[i];
		if (!s.used)
			continue;
		const double dur = s.cfg.duration_ms > 1.0 ? s.cfg.duration_ms : 1.0;
		if ((now_ms - s.born_ms) >= dur)
			s.used = false;
	}
}

int ClickFxModel::active_count() const
{
	int n = 0;
	for (int i = 0; i < kMaxSlots; ++i)
		if (slots_[i].used)
			++n;
	return n;
}

int ClickFxModel::collect(double now_ms, ClickRender *out, int max) const
{
	int written = 0;
	for (int i = 0; i < kMaxSlots && written < max; ++i) {
		const Slot &s = slots_[i];
		if (!s.used)
			continue;

		const double dur = s.cfg.duration_ms > 1.0 ? s.cfg.duration_ms : 1.0;
		double p = (now_ms - s.born_ms) / dur;
		if (p < 0.0)
			p = 0.0;
		if (p > 1.0)
			p = 1.0;

		ClickRender &r = out[written++];
		r.x = s.x;
		r.y = s.y;
		r.progress = p;
		r.eased = ease_apply(s.cfg.easing, p);
		r.size = s.cfg.size;
		r.color = s.cfg.color;
		r.effect = s.cfg.effect;
	}
	return written;
}

} // namespace topscreen::core
