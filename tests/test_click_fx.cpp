#include "core/click_fx.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(click_spawn_and_expiration)
{
	ClickFxModel m;
	ClickConfig left;
	left.duration_ms = 600.0;
	m.set_left(left);

	m.spawn(0.2, 0.3, 0.0, false);
	m.tick(0.0);
	CHECK_EQ(m.active_count(), 1);

	ClickRender out[ClickFxModel::kMaxSlots];
	CHECK_EQ(m.collect(0.0, out, ClickFxModel::kMaxSlots), 1);
	CHECK_NEAR(out[0].progress, 0.0, 1e-9);
	CHECK_NEAR(out[0].x, 0.2, 1e-9);

	/* Half of the duration. */
	CHECK_EQ(m.collect(300.0, out, ClickFxModel::kMaxSlots), 1);
	CHECK_NEAR(out[0].progress, 0.5, 1e-9);

	/* Expired. */
	m.tick(700.0);
	CHECK_EQ(m.active_count(), 0);
}

TEST(click_left_and_right_distinct)
{
	ClickFxModel m;
	ClickConfig l;
	l.effect = static_cast<int>(ClickEffect::Ripple);
	l.color = 0xFFFFFFFF;
	l.duration_ms = 500.0;
	ClickConfig r;
	r.effect = static_cast<int>(ClickEffect::Orb);
	r.color = 0xFF112233;
	r.duration_ms = 500.0;
	m.set_left(l);
	m.set_right(r);

	m.spawn(0.1, 0.1, 0.0, false);
	m.spawn(0.9, 0.9, 0.0, true);

	ClickRender out[ClickFxModel::kMaxSlots];
	const int n = m.collect(100.0, out, ClickFxModel::kMaxSlots);
	CHECK_EQ(n, 2);

	bool found_ripple = false, found_orb = false;
	for (int i = 0; i < n; ++i) {
		if (out[i].effect == static_cast<int>(ClickEffect::Ripple))
			found_ripple = true;
		if (out[i].effect == static_cast<int>(ClickEffect::Orb))
			found_orb = true;
	}
	CHECK(found_ripple);
	CHECK(found_orb);
}

TEST(click_recycles_slot_when_full)
{
	ClickFxModel m;
	ClickConfig c;
	c.duration_ms = 10000.0;
	m.set_left(c);
	m.set_right(c);

	for (int i = 0; i < ClickFxModel::kMaxSlots + 3; ++i)
		m.spawn(0.5, 0.5, static_cast<double>(i), false);

	m.tick(50.0);
	CHECK_EQ(m.active_count(), ClickFxModel::kMaxSlots);
}

TEST(click_progress_saturated)
{
	ClickFxModel m;
	ClickConfig c;
	c.duration_ms = 100.0;
	m.set_left(c);
	m.spawn(0.5, 0.5, 0.0, false);

	ClickRender out[ClickFxModel::kMaxSlots];
	/* Before tick removes it, collecting with a much larger time saturates at 1. */
	m.collect(1000.0, out, ClickFxModel::kMaxSlots);
	CHECK_NEAR(out[0].progress, 1.0, 1e-9);
}
