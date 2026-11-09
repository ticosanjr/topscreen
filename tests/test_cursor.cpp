#include "core/cursor.hpp"
#include "test_framework.hpp"

#include <cmath>

using namespace topscreen::core;

namespace {
void advance(CursorModel &m, double &t, double seconds, double dt = 1.0 / 60.0)
{
	for (double acc = 0.0; acc < seconds; acc += dt) {
		t += dt * 1000.0;
		m.update(t);
	}
}
} // namespace

TEST(cursor_initial_position)
{
	CursorModel m;
	CursorConfig cfg;
	m.configure(cfg);
	m.reset(0.2, 0.3, 0.0);
	const CursorRender r = m.render();
	CHECK_NEAR(r.x, 0.2, 1e-9);
	CHECK_NEAR(r.y, 0.3, 1e-9);
	CHECK_NEAR(r.size, cfg.size, 1e-9);
}

TEST(cursor_follows_target)
{
	CursorModel m;
	CursorConfig cfg;
	m.configure(cfg);
	m.reset(0.0, 0.0, 0.0);
	m.set_target(1.0, 1.0, 0.0);
	double t = 0.0;
	advance(m, t, 2.0);
	const CursorRender r = m.render();
	CHECK_NEAR(r.x, 1.0, 1e-2);
	CHECK_NEAR(r.y, 1.0, 1e-2);
}

TEST(cursor_bundled_styles_are_upright)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.follow_mode = static_cast<int>(CursorFollowMode::Exact);
	cfg.style = static_cast<int>(CursorStyle::MacOs);
	m.configure(cfg);
	m.reset(0.1, 0.1, 0.0);

	double t = 0.0;
	m.update(t);
	CHECK_NEAR(m.render().rotation, 0.0, 1e-9);

	/* Bibata through Dot, including the procedural effects, stay upright. */
	for (CursorStyle style : {CursorStyle::Bibata, CursorStyle::Breeze,
				  CursorStyle::XCursorPro, CursorStyle::MacOs,
				  CursorStyle::Windows, CursorStyle::Neon,
				  CursorStyle::Onyx, CursorStyle::Dot}) {
		cfg.style = static_cast<int>(style);
		m.configure(cfg);
		CHECK_NEAR(m.render().rotation, 0.0, 1e-9);
	}
}

TEST(cursor_custom_styles_keep_the_fixed_30_degree_tilt)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.follow_mode = static_cast<int>(CursorFollowMode::Exact);
	cfg.style = static_cast<int>(CursorStyle::CustomPurpleIris);
	m.configure(cfg);
	m.reset(0.1, 0.1, 0.0);

	double t = 0.0;
	m.update(t);
	CHECK_NEAR(m.render().rotation, 0.5235987756, 1e-9);

	/* Moving in any direction must not change the tilt (no sway). */
	m.set_target(0.9, 0.2, 0.0);
	advance(m, t, 1.0);
	CHECK_NEAR(m.render().rotation, 0.5235987756, 1e-9);
	m.set_target(0.2, 0.9, 0.0);
	advance(m, t, 1.0);
	CHECK_NEAR(m.render().rotation, 0.5235987756, 1e-9);
}

TEST(cursor_rotation_offset_adds_to_the_style_baseline)
{
	constexpr double kPi = 3.14159265358979323846;
	CursorModel m;
	CursorConfig cfg;
	cfg.follow_mode = static_cast<int>(CursorFollowMode::Exact);
	cfg.style = static_cast<int>(CursorStyle::Bibata);
	cfg.rotation_offset_deg = 15.0;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);
	m.update(0.0);
	CHECK_NEAR(m.render().rotation, 15.0 * kPi / 180.0, 1e-9);

	/* Custom baseline is 30°, so -30° makes it upright. */
	cfg.style = static_cast<int>(CursorStyle::CustomPurpleIris);
	cfg.rotation_offset_deg = -30.0;
	m.configure(cfg);
	m.update(1.0);
	CHECK_NEAR(m.render().rotation, 0.0, 1e-9);

	cfg.rotation_offset_deg = 10.0;
	m.configure(cfg);
	m.update(2.0);
	CHECK_NEAR(m.render().rotation, 40.0 * kPi / 180.0, 1e-9);
}

TEST(cursor_idle_reduces_opacity)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.hide_idle = true;
	cfg.idle_delay_ms = 1000.0;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);

	double t = 0.0;
	advance(m, t, 0.5); /* before the delay */
	CHECK(m.render().opacity > 0.99);

	advance(m, t, 2.0); /* well past delay + fade */
	CHECK(m.render().opacity < 0.01);
	CHECK(!m.visible());
}

TEST(cursor_without_hide_idle_keeps_opacity)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.hide_idle = false;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);
	double t = 0.0;
	advance(m, t, 5.0);
	CHECK_NEAR(m.render().opacity, 1.0, 1e-9);
}

TEST(cursor_disabled_invisible)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.enabled = false;
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);
	CHECK(!m.render().visible);
	CHECK_NEAR(m.render().opacity, 0.0, 1e-9);
}

TEST(cursor_exact_mode_has_no_lag)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.follow_mode = static_cast<int>(CursorFollowMode::Exact);
	m.configure(cfg);
	m.reset(0.0, 0.0, 0.0);
	m.set_target(0.8, 0.6, 0.0);
	m.update(0.0);
	CHECK_NEAR(m.render().x, 0.8, 1e-9);
	CHECK_NEAR(m.render().y, 0.6, 1e-9);
}

TEST(cursor_glide_ab_is_a_smooth_point_to_point)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.follow_mode = static_cast<int>(CursorFollowMode::GlideAB);
	cfg.glide_ms = 200.0;
	m.configure(cfg);
	m.reset(0.0, 0.0, 0.0);
	m.set_target(1.0, 1.0, 0.0);

	m.update(0.0);
	CHECK_NEAR(m.render().x, 0.0, 1e-9);
	m.update(100.0);
	/* The path is a subtle quadratic curve, not a straight line. */
	CHECK(std::fabs(m.render().x - 0.5) > 0.01);
	m.update(200.0);
	CHECK_NEAR(m.render().x, 1.0, 1e-9);

	/* A target that arrives while gliding is accepted at the next leg. */
	m.set_target(0.2, 0.2, 200.0);
	m.update(200.0);
	m.update(400.0);
	CHECK_NEAR(m.render().x, 0.2, 1e-9);
}

TEST(cursor_link_uses_the_secondary_style)
{
	CursorModel m;
	CursorConfig cfg;
	cfg.style = static_cast<int>(CursorStyle::Bibata);
	cfg.secondary_style = static_cast<int>(CursorStyle::BibataHover);
	m.configure(cfg);
	m.reset(0.5, 0.5, 0.0);

	CHECK_EQ(m.render().style, static_cast<int>(CursorStyle::Bibata));
	m.set_link(true);
	CHECK_EQ(m.render().style, static_cast<int>(CursorStyle::BibataHover));
	m.set_link(false);
	CHECK_EQ(m.render().style, static_cast<int>(CursorStyle::Bibata));

	cfg.secondary_style = -1;
	m.configure(cfg);
	m.set_link(true);
	CHECK_EQ(m.render().style, static_cast<int>(CursorStyle::Bibata));
}

TEST(cursor_link_name_classifier)
{
	CHECK(cursor_name_is_link("hand"));
	CHECK(cursor_name_is_link("hand2"));
	CHECK(cursor_name_is_link("pointer"));
	CHECK(cursor_name_is_link("link"));
	CHECK(cursor_name_is_link("LINK"));
	CHECK(cursor_name_is_link("my-link-cursor"));
	CHECK(!cursor_name_is_link("left_ptr"));
	CHECK(!cursor_name_is_link("arrow"));
	CHECK(!cursor_name_is_link(""));
}
