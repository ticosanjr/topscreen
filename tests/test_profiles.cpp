#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "test_framework.hpp"

#include <string>

using namespace topscreen::core;

TEST(profiles_seed_a_is_default)
{
	const Config a = make_profile_seed(0);
	const Config def;
	CHECK_EQ(a.click_left.color, def.click_left.color);
	CHECK_NEAR(a.zoom.amount, def.zoom.amount, 1e-9);
	CHECK_EQ(a.zoom.mode, def.zoom.mode);
}

TEST(profiles_seed_b_changes_color_and_zoom)
{
	const Config b = make_profile_seed(1);
	CHECK_EQ(b.click_left.color, 0xFF00FF00u);
	CHECK_EQ(b.click_right.color, 0xFF00A5FFu);
	CHECK_NEAR(b.zoom.amount, 2.0, 1e-9);
}

TEST(profiles_seed_c_and_d)
{
	const Config c = make_profile_seed(2);
	CHECK_EQ(c.click_left.color, 0xFFFF5C7Cu);
	CHECK_NEAR(c.zoom.amount, 1.4, 1e-9);
	CHECK_EQ(c.bg.mode, static_cast<int>(BgMode::Vertical));

	const Config d = make_profile_seed(3);
	CHECK_EQ(d.zoom.mode, static_cast<int>(ZoomMode::Off));
	CHECK_EQ(d.click_left.color, 0xFF00D6FFu);
}

TEST(profiles_seed_differ_from_each_other)
{
	/* No pair of slots may be identical (otherwise switching would be invisible). */
	const int count = keys::kProfileCount;
	for (int i = 0; i < count; ++i) {
		for (int j = i + 1; j < count; ++j) {
			const Config a = make_profile_seed(i);
			const Config b = make_profile_seed(j);
			const bool same = a.click_left.color == b.click_left.color &&
					  a.click_right.color == b.click_right.color &&
					  a.zoom.amount == b.zoom.amount && a.zoom.mode == b.zoom.mode;
			CHECK(!same);
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
		/* Non-empty and never repeated (the original complaint was that all
		 * profiles looked the same). */
		CHECK(!std::string(profile_label(i)).empty());
		for (int j = i + 1; j < keys::kProfileCount; ++j)
			CHECK(std::string(profile_label(i)) != std::string(profile_label(j)));
	}
}
