#include "properties/ts-option-schema.hpp"

#include "test_framework.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <set>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif
#ifndef TOPSCREEN_SOURCE_DIR
#define TOPSCREEN_SOURCE_DIR "."
#endif

namespace {

std::set<std::string> string_values(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	const std::string text = ss.str();
	std::set<std::string> values;
	size_t pos = 0;
	while ((pos = text.find('"', pos)) != std::string::npos) {
		const auto end = text.find('"', pos + 1);
		if (end == std::string::npos)
			break;
		if (end > pos + 1)
			values.insert(text.substr(pos + 1, end - pos - 1));
		pos = end + 1;
	}
	return values;
}

std::set<std::string> tooltip_names(const std::string &text)
{
	std::set<std::string> names;
	const std::string needle = "\"Tooltip.";
	size_t pos = 0;
	while ((pos = text.find(needle, pos)) != std::string::npos) {
		pos += needle.size();
		const auto end = text.find('"', pos);
		if (end == std::string::npos)
			break;
		names.insert("Tooltip." + text.substr(pos, end - pos));
	}
	return names;
}

std::set<std::string> property_tooltips(const std::string &src_dir)
{
	std::set<std::string> names;
	namespace fs = std::filesystem;
	const auto add_file = [&](const fs::path &path) {
		std::ifstream in(path);
		std::ostringstream ss;
		ss << in.rdbuf();
		for (const auto &n : tooltip_names(ss.str()))
			names.insert(n);
	};
	add_file(fs::path(src_dir) / "ts-properties.cpp");
	for (const auto &entry : fs::directory_iterator(fs::path(src_dir) / "properties")) {
		if (entry.is_regular_file() && entry.path().extension() == ".cpp")
			add_file(entry.path());
	}
	return names;
}

std::set<std::string> locale_keys(const std::string &path)
{
	std::ifstream in(path);
	std::set<std::string> keys;
	std::string line;
	while (std::getline(in, line)) {
		if (line.empty() || line[0] == '#' || line.find('=') == std::string::npos)
			continue;
		keys.insert(line.substr(0, line.find('=')));
	}
	return keys;
}

} // namespace

/* The option registry is the single place for key/section/tooltip metadata.
 * Every entry must be unique and point at a tooltip present in both locales. */
TEST(option_schema_is_unique_and_localized)
{
	using namespace topscreen::options;

	const std::set<std::string> en =
		locale_keys(std::string(TOPSCREEN_TEST_DATA_DIR) + "/locale/en-US.ini");
	const std::set<std::string> pt =
		locale_keys(std::string(TOPSCREEN_TEST_DATA_DIR) + "/locale/pt-BR.ini");
	CHECK(!en.empty());
	CHECK(!pt.empty());
	CHECK(kSpecCount >= 100);

	std::set<std::string> seen;
	for (std::size_t i = 0; i < kSpecCount; ++i) {
		const Spec &spec = kSpecs[i];
		++tst::checks();
		if (!spec.key || !*spec.key) {
			tst::fail(__FILE__, __LINE__, "option spec without a key");
			continue;
		}
		++tst::checks();
		if (!seen.insert(spec.key).second)
			tst::fail(__FILE__, __LINE__,
				  std::string("duplicate option key: ") + spec.key);
		++tst::checks();
		if (!spec.tooltip || !*spec.tooltip) {
			tst::fail(__FILE__, __LINE__,
				  std::string("option without a tooltip key: ") + spec.key);
			continue;
		}
		++tst::checks();
		if (!en.count(spec.tooltip))
			tst::fail(__FILE__, __LINE__,
				  std::string("tooltip missing from en-US: ") + spec.tooltip);
		++tst::checks();
		if (!pt.count(spec.tooltip))
			tst::fail(__FILE__, __LINE__,
				  std::string("tooltip missing from pt-BR: ") + spec.tooltip);
	}
}

/* Every tooltip used by the filter Properties builders must be declared in the
 * schema, so the help text and the option metadata cannot drift apart. */
TEST(option_schema_covers_property_tooltips)
{
	using namespace topscreen::options;
	std::set<std::string> registered;
	for (std::size_t i = 0; i < kSpecCount; ++i)
		registered.insert(kSpecs[i].tooltip);

	const std::set<std::string> used =
		property_tooltips(std::string(TOPSCREEN_SOURCE_DIR) + "/src");
	CHECK(!used.empty());
	for (const auto &name : used) {
		++tst::checks();
		if (!registered.count(name))
			tst::fail(__FILE__, __LINE__,
				  "tooltip missing from the option schema: " + name);
	}
}

/* Every config key exposed by the filter must be in the registry. Legacy or
 * internal keys and the separate webcam-filter options are excluded. */
TEST(option_schema_covers_config_keys)
{
	using namespace topscreen::options;
	std::set<std::string> registered;
	for (std::size_t i = 0; i < kSpecCount; ++i)
		registered.insert(kSpecs[i].key);

	const std::set<std::string> allowed = {
		"bg_mode", "click_sound_file", "click_sound_left", "click_sound_right",
		"spotlight_blur_enabled", "profile_seed_version"};

	const std::set<std::string> keys =
		string_values(std::string(TOPSCREEN_SOURCE_DIR) + "/src/core/keys.hpp");
	CHECK(!keys.empty());
	for (const auto &key : keys) {
		if (allowed.count(key) || key.rfind("profile_slot_", 0) == 0 ||
		    key.rfind("webcam_", 0) == 0)
			continue;
		++tst::checks();
		if (!registered.count(key))
			tst::fail(__FILE__, __LINE__,
				  "config key missing from the option schema: " + key);
	}
}
