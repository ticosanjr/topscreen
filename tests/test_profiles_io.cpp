#include "core/profiles_io.hpp"

#include "test_framework.hpp"

#include <string>

using namespace topscreen::core;

namespace {

std::string valid_bundle(int current = 0)
{
	std::string s = "{\"topscreen_profiles\":1,\"plugin\":\"TopScreen\",\"current\":" +
			std::to_string(current) + ",";
	for (int i = 0; i < 10; ++i) {
		s += "\"profile_slot_" + std::to_string(i) + "\":{\"effects_enabled\":true}";
		if (i < 9)
			s += ",";
	}
	s += "}";
	return s;
}

bool rejects(const std::string &text)
{
	ProfileBundleInfo info;
	std::string error;
	const bool ok = profile_bundle_validate(text, &info, &error);
	CHECK(!error.empty());
	return !ok;
}

} // namespace

TEST(profile_bundle_accepts_a_full_backup)
{
	ProfileBundleInfo info;
	std::string error;
	REQUIRE(profile_bundle_validate(valid_bundle(3), &info, &error));
	CHECK_EQ(info.current, 3);
	CHECK_EQ(info.slot_count, 10);
	CHECK(error.empty());
}

TEST(profile_bundle_accepts_extra_keys_from_newer_versions)
{
	std::string text = valid_bundle(0);
	text.insert(text.size() - 1, ",\"future_key\":{\"x\":1}");
	ProfileBundleInfo info;
	std::string error;
	REQUIRE(profile_bundle_validate(text, &info, &error));
	CHECK_EQ(info.slot_count, 10);
}

TEST(profile_bundle_rejects_broken_json)
{
	CHECK(rejects("{not json"));
	CHECK(rejects(""));
	CHECK(rejects("[]"));
}

TEST(profile_bundle_rejects_wrong_markers)
{
	std::string text = valid_bundle(0);
	CHECK(rejects(text.replace(text.find("\"topscreen_profiles\":1"),
				   std::string("\"topscreen_profiles\":1").size(),
				   "\"topscreen_profiles\":2")));

	text = valid_bundle(0);
	CHECK(rejects(text.replace(text.find("TopScreen"), std::string("TopScreen").size(),
				   "OtherPlugin")));
}

TEST(profile_bundle_accepts_missing_or_empty_slots)
{
	std::string text = valid_bundle(0);
	const std::string slot = "\"profile_slot_4\":{\"effects_enabled\":true},";
	text.erase(text.find(slot), slot.size());
	ProfileBundleInfo info;
	std::string error;
	REQUIRE(profile_bundle_validate(text, &info, &error));
	CHECK_EQ(info.slot_count, 9);

	text = valid_bundle(0);
	text.replace(text.find("\"profile_slot_4\":{\"effects_enabled\":true}"),
		     std::string("\"profile_slot_4\":{\"effects_enabled\":true}").size(),
		     "\"profile_slot_4\":{}");
	REQUIRE(profile_bundle_validate(text, &info, &error));
	CHECK_EQ(info.slot_count, 10);
}

TEST(profile_bundle_defaults_a_missing_current_index)
{
	std::string text = valid_bundle(0);
	text.erase(text.find("\"current\":0,"), std::string("\"current\":0,").size());
	ProfileBundleInfo info;
	std::string error;
	REQUIRE(profile_bundle_validate(text, &info, &error));
	CHECK_EQ(info.current, 0);
	CHECK_EQ(info.slot_count, 10);
}

TEST(profile_bundle_rejects_invalid_current_index)
{
	CHECK(rejects(valid_bundle(-1)));
	CHECK(rejects(valid_bundle(10)));

	std::string text = valid_bundle(0);
	text.replace(text.find("\"current\":0"), std::string("\"current\":0").size(),
		     "\"current\":1.5");
	CHECK(rejects(text));
}

TEST(profile_bundle_rejects_an_oversized_file)
{
	const std::string huge(kMaxProfileBundleBytes + 1, ' ');
	ProfileBundleInfo info;
	std::string error;
	CHECK(!profile_bundle_validate(huge, &info, &error));
	CHECK(!error.empty());
}
