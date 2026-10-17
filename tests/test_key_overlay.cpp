#include "core/key_overlay.hpp"
#include "test_framework.hpp"

using namespace topscreen::core;

TEST(key_format_combo)
{
	CHECK_EQ(format_combo({}, "K"), std::string("K"));
	CHECK_EQ(format_combo({"Ctrl"}, "K"), std::string("Ctrl + K"));
	CHECK_EQ(format_combo({"Ctrl", "Shift"}, "K"), std::string("Ctrl + Shift + K"));
	CHECK_EQ(format_combo({"Ctrl"}, ""), std::string("Ctrl"));
	CHECK_EQ(format_combo({}, ""), std::string(""));
	CHECK_EQ(format_combo({"Ctrl", "", "Alt"}, "X"), std::string("Ctrl + Alt + X"));
}

TEST(key_press_and_collect)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	m.configure(cfg);

	m.press("A", false, 0.0);
	m.press("B", false, 10.0);
	CHECK_EQ(m.active_count(), 2);

	KeyRender out[8];
	const int n = m.collect(10.0, out, 8);
	CHECK_EQ(n, 2);
	CHECK_EQ(out[0].label, std::string("A"));
	CHECK_EQ(out[1].label, std::string("B"));
}

TEST(key_expires_by_ttl)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.ttl_ms = 1000.0;
	m.configure(cfg);

	m.press("A", false, 0.0);
	m.tick(500.0);
	CHECK_EQ(m.active_count(), 1);
	m.tick(1000.0);
	CHECK_EQ(m.active_count(), 0);
}

TEST(key_repeat_appends_each_keystroke)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.ttl_ms = 1000.0;
	m.configure(cfg);

	/* Typing "aa" shows two letters (repeats included). */
	m.press("a", false, 0.0);
	m.press("a", false, 900.0);
	CHECK_EQ(m.active_count(), 2);

	/* Each letter expires on its own TTL. */
	m.tick(1500.0);
	CHECK_EQ(m.active_count(), 1); /* 1500 - 900 = 600 < ttl */
	m.tick(2000.0);
	CHECK_EQ(m.active_count(), 0); /* 2000 - 900 = 1100 >= ttl */
}

TEST(key_case_is_preserved)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	m.configure(cfg);

	m.press("a", false, 0.0);

	KeyRender out[8];
	const int n = m.collect(0.0, out, 8);
	REQUIRE(n == 1);
	CHECK_EQ(out[0].label, std::string("a"));
}

TEST(key_modifiers_only_filters)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.modifiers_only = true;
	m.configure(cfg);

	m.press("A", false, 0.0);
	CHECK_EQ(m.active_count(), 0);
	m.press("Ctrl", true, 0.0);
	CHECK_EQ(m.active_count(), 1);
}

TEST(key_key_limit)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.max_keys = 3;
	m.configure(cfg);

	m.press("A", false, 0.0);
	m.press("B", false, 1.0);
	m.press("C", false, 2.0);
	m.press("D", false, 3.0);
	CHECK_EQ(m.active_count(), 3);

	KeyRender out[8];
	m.collect(3.0, out, 8);
	CHECK_EQ(out[0].label, std::string("B")); /* "A" was discarded */
	CHECK_EQ(out[2].label, std::string("D"));
}

TEST(key_clear)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	m.configure(cfg);
	m.press("A", false, 0.0);
	m.clear();
	CHECK_EQ(m.active_count(), 0);
}

TEST(key_disabled_ignores)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.enabled = false;
	m.configure(cfg);
	m.press("A", false, 0.0);
	CHECK_EQ(m.active_count(), 0);
}
