#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "test_framework.hpp"

#include <string>

using namespace topscreen::core;

TEST(profiles_seed_a_is_balanced_default)
{
	const Config a = make_profile_seed(0);
	CHECK(a.effects_enabled);
	CHECK_EQ(a.bg.mode, static_cast<int>(BgMode::Gradient));
	CHECK_EQ(a.cursor.style, static_cast<int>(CursorStyle::MacOs));
	CHECK_EQ(a.zoom.mode, static_cast<int>(ZoomMode::Auto));
	CHECK_NEAR(a.zoom.amount, 1.35, 1e-9);
	/* Typed keys are opt-in on new installs (privacy). */
	CHECK(!a.keys.enabled);
	CHECK(a.highlight.enabled);
	CHECK(!a.spotlight.enabled);
	CHECK(!a.magnifier.enabled);
}

TEST(profiles_seed_b_is_clean_and_minimal)
{
	const Config b = make_profile_seed(1);
	CHECK_EQ(b.bg.mode, static_cast<int>(BgMode::Transparent));
	CHECK_EQ(b.zoom.mode, static_cast<int>(ZoomMode::Off));
	CHECK(!b.keys.enabled);
	CHECK(!b.highlight.enabled);
	CHECK(b.click_sound.file_right.empty());
}

TEST(profiles_seed_c_is_presentation)
{
	const Config c = make_profile_seed(2);
	CHECK_EQ(c.bg.mode, static_cast<int>(BgMode::Gradient));
	CHECK_NEAR(c.zoom.amount, 1.25, 1e-9);
	CHECK(c.keys.enabled);
	CHECK(c.highlight.enabled);
	CHECK_EQ(c.highlight.mode, static_cast<int>(HighlightMode::Box));
}

TEST(profiles_seed_d_is_streaming)
{
	const Config d = make_profile_seed(3);
	CHECK_EQ(d.bg.mode, static_cast<int>(BgMode::Solid));
	CHECK_NEAR(d.zoom.amount, 1.60, 1e-9);
	CHECK(!d.keys.enabled);
	CHECK(!d.highlight.enabled);
	CHECK(d.click_left.effect != d.click_right.effect);
}

TEST(profiles_seed_e_and_f_use_focus_effects)
{
	const Config e = make_profile_seed(4);
	CHECK(e.spotlight.enabled);
	CHECK(!e.magnifier.enabled);

	const Config f = make_profile_seed(5);
	CHECK(f.magnifier.enabled);
	CHECK(!f.spotlight.enabled);
	CHECK(f.magnifier.shadow_enabled);
}

TEST(profiles_seed_j_is_full_passthrough)
{
	const Config j = make_profile_seed(9);
	CHECK(!j.effects_enabled);
	CHECK(!j.bg.enabled);
	CHECK(!j.cursor.enabled);
	CHECK_EQ(j.zoom.mode, static_cast<int>(ZoomMode::Off));
	CHECK(!j.click_sound.enabled);
	CHECK(!j.keys.enabled);
	CHECK(!j.highlight.enabled);
	CHECK(!j.spotlight.enabled);
	CHECK(!j.magnifier.enabled);
}

TEST(profiles_seed_differ_from_each_other)
{
	/* No pair of slots may be identical, otherwise switching is invisible. */
	const int count = keys::kProfileCount;
	auto signature = [](const Config &c) {
		return std::to_string(c.effects_enabled) + "|" + std::to_string(c.bg.enabled) +
		       "|" + std::to_string(c.bg.mode) + "|" + std::to_string(c.cursor.enabled) +
		       "|" + std::to_string(c.cursor.style) + "|" + std::to_string(c.zoom.mode) +
		       "|" + std::to_string(c.zoom.amount) + "|" +
		       std::to_string(c.click_left.color) + "|" +
		       std::to_string(c.click_right.color) + "|" +
		       std::to_string(c.spotlight.enabled) + "|" +
		       std::to_string(c.magnifier.enabled) + "|" +
		       std::to_string(c.keys.enabled) + "|" +
		       std::to_string(c.highlight.enabled);
	};
	for (int i = 0; i < count; ++i) {
		for (int j = i + 1; j < count; ++j) {
			CHECK(signature(make_profile_seed(i)) != signature(make_profile_seed(j)));
		}
	}
}

TEST(profiles_seed_out_of_range_index)
{
	/* Must not break and must cycle. */
	const Config a = make_profile_seed(0);
	const Config wrap = make_profile_seed(keys::kProfileCount);
	CHECK_EQ(wrap.click_left.color, a.click_left.color);
	CHECK_EQ(profile_index(keys::kProfileCount), 0);
	CHECK_EQ(profile_index(-1), keys::kProfileCount - 1);
}

TEST(profiles_distinct_labels)
{
	const char *expected[keys::kProfileCount] = {"Profile A", "Profile B", "Profile C", "Profile D",
						    "Profile E", "Profile F", "Profile G", "Profile H",
						    "Profile I", "Profile J"};
	for (int i = 0; i < keys::kProfileCount; ++i) {
		CHECK_EQ(std::string(profile_label(i)), std::string(expected[i]));
		CHECK(!std::string(profile_label(i)).empty());
		for (int j = i + 1; j < keys::kProfileCount; ++j)
			CHECK(std::string(profile_label(i)) != std::string(profile_label(j)));
	}
}
