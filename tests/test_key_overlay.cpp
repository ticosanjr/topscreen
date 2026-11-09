#include "core/key_overlay.hpp"
#include "test_framework.hpp"

#include <string_view>
#include <type_traits>

using namespace topscreen::core;

static_assert(std::is_trivially_copyable_v<KeyLabel>);
static_assert(std::is_trivially_copyable_v<KeyRender>);


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
	CHECK_EQ(out[0].label.view(), std::string_view("A"));
	CHECK_EQ(out[1].label.view(), std::string_view("B"));
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
	CHECK_EQ(out[0].label.view(), std::string_view("a"));
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

TEST(key_special_keys_can_be_ignored)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.show_special = false;
	m.configure(cfg);

	m.press("A", false, 0.0);
	m.press("1", false, 0.0);
	m.press("@", false, 0.0);
	m.press("Space", false, 0.0);
	m.press("Left", false, 0.0);
	m.press("Ctrl", true, 0.0);
	CHECK_EQ(m.active_count(), 4);

	m.press("Esc", false, 0.0);
	CHECK_EQ(m.active_count(), 4);

	KeyRender out[8];
	const int n = m.collect(0.0, out, 8);
	REQUIRE(n == 4);
	CHECK_EQ(out[0].label.view(), std::string_view("A"));
	CHECK_EQ(out[1].label.view(), std::string_view("1"));
	CHECK_EQ(out[2].label.view(), std::string_view("@"));
	CHECK_EQ(out[3].label.view(), std::string_view(" "));
}

TEST(key_modifiers_only_wins_over_special_filter)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.modifiers_only = true;
	cfg.show_special = false;
	m.configure(cfg);

	m.press("A", false, 0.0);
	m.press("Ctrl", true, 0.0);
	m.press("Shift", true, 0.0);
	CHECK_EQ(m.active_count(), 2);
}

TEST(key_special_label_classifier)
{
	CHECK(!is_special_key_label("Space"));
	CHECK(is_special_key_label("Backspace"));
	CHECK(is_special_key_label("Enter"));
	CHECK(is_special_key_label("Tab"));
	CHECK(is_special_key_label("Esc"));
	CHECK(is_special_key_label("Left"));
	CHECK(!is_special_key_label("Ctrl"));
	CHECK(!is_special_key_label("Shift"));
	CHECK(!is_special_key_label("Win"));
	CHECK(!is_special_key_label("A"));
	CHECK(!is_special_key_label("1"));
	CHECK(!is_special_key_label("@"));
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
	CHECK_EQ(out[0].label.view(), std::string_view("B")); /* "A" was discarded */
	CHECK_EQ(out[2].label.view(), std::string_view("D"));
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

TEST(key_label_truncates_at_renderer_limit)
{
	KeyLabel label;
	label.assign(std::string(100, 'x'));
	CHECK_EQ(label.view().size(), KeyLabel::kCapacity - 1);
	CHECK_EQ(label.view(), std::string_view(std::string(KeyLabel::kCapacity - 1, 'x')));

	label.assign("Backspace");
	CHECK_EQ(label.view(), std::string_view("Backspace"));

	label.clear();
	CHECK(label.view().empty());
}

TEST(key_collect_preserves_long_labels_up_to_limit)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.max_keys = 2;
	m.configure(cfg);

	std::string long_label(80, 'k');
	m.press(long_label, false, 0.0);

	KeyRender out[2];
	const int n = m.collect(0.0, out, 2);
	REQUIRE(n == 1);
	CHECK_EQ(out[0].label.view().size(), KeyLabel::kCapacity - 1);
	CHECK_EQ(out[0].label.view(), std::string_view(long_label.substr(0, KeyLabel::kCapacity - 1)));
}

TEST(key_shortcuts_combine_modifiers)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.show_shortcuts = true;
	m.configure(cfg);

	m.press("Ctrl", true, 0.0, KeyModCtrl);
	m.press("C", false, 1.0, KeyModCtrl | KeyModShift);
	m.press("V", false, 2.0, KeyModCtrl);

	KeyRender out[8];
	const int n = m.collect(2.0, out, 8);
	REQUIRE(n == 2);
	CHECK_EQ(out[0].label.view(), std::string_view("Ctrl + Shift + C"));
	CHECK_EQ(out[1].label.view(), std::string_view("Ctrl + V"));
}

TEST(key_special_names_are_text_and_space_is_blank)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.show_special = true;
	cfg.show_shortcuts = false;
	m.configure(cfg);

	m.press("Shift", true, 0.0);
	m.press("Space", false, 1.0);
	m.press("Left", false, 2.0);

	KeyRender out[8];
	const int n = m.collect(2.0, out, 8);
	REQUIRE(n == 3);
	CHECK_EQ(out[0].label.view(), std::string_view("Shift"));
	CHECK_EQ(out[1].label.view(), std::string_view(" "));
	CHECK_EQ(out[2].label.view(), std::string_view("Left"));
}

TEST(key_shortcuts_suppress_standalone_modifiers_even_with_special)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.show_shortcuts = true;
	cfg.show_special = true;
	m.configure(cfg);

	/* Duplicate modifier events from a platform must not create entries. */
	m.press("Ctrl", true, 0.0, KeyModCtrl);
	m.press("Ctrl", true, 0.0, KeyModCtrl);
	m.press("C", false, 1.0, KeyModCtrl);
	CHECK_EQ(m.active_count(), 1);

	KeyRender out[8];
	const int n = m.collect(1.0, out, 8);
	REQUIRE(n == 1);
	CHECK_EQ(out[0].label.view(), std::string_view("Ctrl + C"));
}

TEST(key_without_shortcuts_shows_modifier_once)
{
	KeyOverlayModel m;
	KeyOverlayConfig cfg;
	cfg.show_shortcuts = false;
	cfg.show_special = false;
	m.configure(cfg);

	m.press("Ctrl", true, 0.0, KeyModCtrl);
	m.press("Ctrl", true, 1.0, KeyModCtrl);
	m.press("C", false, 2.0, KeyModCtrl);
	CHECK_EQ(m.active_count(), 2);
}
