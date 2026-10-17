#include "test_framework.hpp"

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif
#ifndef TOPSCREEN_SOURCE_DIR
#define TOPSCREEN_SOURCE_DIR "."
#endif

namespace fs = std::filesystem;

namespace {

std::set<std::string> ini_keys(const std::string &path)
{
	std::set<std::string> keys;
	std::ifstream in(path);
	std::string line;
	while (std::getline(in, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[')
			continue;
		const auto eq = line.find('=');
		if (eq == std::string::npos)
			continue;
		std::string key = line.substr(0, eq);
		while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
			key.pop_back();
		if (!key.empty())
			keys.insert(key);
	}
	return keys;
}

/* Collects the keys used by obs_module_text("...") across the sources. */
std::set<std::string> code_locale_keys(const std::string &src_dir)
{
	std::set<std::string> keys;
	const std::string needle = "obs_module_text(\"";

	if (!fs::exists(src_dir))
		return keys;

	for (const auto &entry : fs::recursive_directory_iterator(src_dir)) {
		if (!entry.is_regular_file())
			continue;
		const auto ext = entry.path().extension();
		if (ext != ".cpp" && ext != ".hpp")
			continue;

		std::ifstream in(entry.path());
		std::ostringstream ss;
		ss << in.rdbuf();
		const std::string text = ss.str();

		size_t pos = 0;
		while ((pos = text.find(needle, pos)) != std::string::npos) {
			pos += needle.size();
			const auto end = text.find('"', pos);
			if (end == std::string::npos)
				break;
			keys.insert(text.substr(pos, end - pos));
			pos = end;
		}
	}
	return keys;
}

} // namespace

/* Every key used in the code must exist in en-US.ini (otherwise OBS shows the
 * raw key in the UI). */
TEST(locale_covers_obs_module_text)
{
	const auto code = code_locale_keys(std::string(TOPSCREEN_SOURCE_DIR) + "/src");
	CHECK(!code.empty());

	const auto en = ini_keys(std::string(TOPSCREEN_TEST_DATA_DIR) + "/locale/en-US.ini");
	CHECK(!en.empty());

	for (const auto &k : code) {
		++tst::checks();
		if (en.find(k) == en.end())
			tst::fail(__FILE__, __LINE__, "key missing from en-US.ini: " + k);
	}
}

/* en-US.ini must not carry leftover, unused keys. */
TEST(locale_en_has_no_orphan_keys)
{
	const auto code = code_locale_keys(std::string(TOPSCREEN_SOURCE_DIR) + "/src");
	const auto en = ini_keys(std::string(TOPSCREEN_TEST_DATA_DIR) + "/locale/en-US.ini");

	CHECK(!code.empty());
	CHECK(!en.empty());

	for (const auto &k : en) {
		++tst::checks();
		if (code.find(k) == code.end())
			tst::fail(__FILE__, __LINE__, "key in en-US.ini is not used by the code: " + k);
	}
}
