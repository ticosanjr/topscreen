#include "test_framework.hpp"

#include "hotkeys/ts-hotkey-actions.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_SOURCE_DIR
#define TOPSCREEN_SOURCE_DIR "."
#endif

namespace {

std::string read_text(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

/* Extracts the values of the constants declared in src/core/keys.hpp. */
std::set<std::string> keys_from_header(const std::string &path)
{
	std::set<std::string> keys;
	std::ifstream in(path);
	std::string line;
	while (std::getline(in, line)) {
		const auto q1 = line.find('"');
		if (q1 == std::string::npos)
			continue;
		const auto q2 = line.find('"', q1 + 1);
		if (q2 == std::string::npos)
			continue;
		keys.insert(line.substr(q1 + 1, q2 - q1 - 1));
	}
	return keys;
}

bool contains(const std::string &haystack, const std::string &needle)
{
	return haystack.find(needle) != std::string::npos;
}

} // namespace

/* The configuration reference must document EVERY key. */
TEST(docs_document_every_key)
{
	const std::string keys_hpp = std::string(TOPSCREEN_SOURCE_DIR) + "/src/core/keys.hpp";
	const std::string config_md = std::string(TOPSCREEN_SOURCE_DIR) + "/docs/CONFIGURATION.md";

	const auto keys = keys_from_header(keys_hpp);
	CHECK(!keys.empty());

	const std::string doc = read_text(config_md);
	CHECK(!doc.empty());

	for (const auto &k : keys) {
		++tst::checks();
		if (!contains(doc, k))
			tst::fail(__FILE__, __LINE__, "key missing from docs/CONFIGURATION.md: " + k);
	}
}

/* Every hotkey declared in the code must be listed in the reference. */
TEST(docs_reference_lists_hotkeys)
{
	const std::string header = read_text(std::string(TOPSCREEN_SOURCE_DIR) +
					     "/src/hotkeys/ts-hotkey-actions.hpp");
	const std::string doc = read_text(std::string(TOPSCREEN_SOURCE_DIR) + "/docs/CONFIGURATION.md");
	CHECK(!doc.empty());
	CHECK(!header.empty());

	for (int i = 0; i < topscreen::kHotkeyCount; ++i) {
		const std::string id = topscreen::kHotkeyNames[i];
		++tst::checks();
		if (!contains(header, id))
			tst::fail(__FILE__, __LINE__, "hotkey action missing from the code: " + id);

		++tst::checks();
		if (!contains(doc, id))
			tst::fail(__FILE__, __LINE__, "hotkey missing from docs/CONFIGURATION.md: " + id);
	}
}

/* One version, two files: CMake builds it and buildspec declares it. Keep
 * them equal so the OBS log, the filter properties and the packages agree. */
TEST(version_matches_buildspec)
{
	const std::string cmake = read_text(std::string(TOPSCREEN_SOURCE_DIR) + "/CMakeLists.txt");
	const std::string spec = read_text(std::string(TOPSCREEN_SOURCE_DIR) + "/buildspec.json");
	REQUIRE(!cmake.empty());
	REQUIRE(!spec.empty());

	const std::string cmake_needle = "project(TopScreen VERSION ";
	const auto cp = cmake.find(cmake_needle);
	REQUIRE(cp != std::string::npos);
	const auto ce = cmake.find_first_of(" \n\r", cp + cmake_needle.size());
	const std::string cmake_version = cmake.substr(cp + cmake_needle.size(),
						       ce - (cp + cmake_needle.size()));

	const std::string spec_needle = "\"version\": \"";
	const auto sp = spec.find(spec_needle);
	REQUIRE(sp != std::string::npos);
	const auto se = spec.find('"', sp + spec_needle.size());
	const std::string spec_version = spec.substr(sp + spec_needle.size(),
						     se - (sp + spec_needle.size()));

	CHECK_EQ(cmake_version, spec_version);
	CHECK(!cmake_version.empty());
}
