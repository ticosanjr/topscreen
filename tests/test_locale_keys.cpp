#include "test_framework.hpp"

#include "core/cursor_catalog.hpp"
#include "hotkeys/ts-hotkey-actions.hpp"

#include <filesystem>
#include <fstream>
#include <map>
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

std::map<std::string, std::string> ini_values(const std::string &path)
{
	std::map<std::string, std::string> values;
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
			values[key] = line.substr(eq + 1);
	}
	return values;
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
	auto code = code_locale_keys(std::string(TOPSCREEN_SOURCE_DIR) + "/src");
	const auto en = ini_keys(std::string(TOPSCREEN_TEST_DATA_DIR) + "/locale/en-US.ini");

	/* Cursor labels live in the catalog table (not inside an
	 * obs_module_text("...") call), so add them explicitly. */
	for (const auto &entry : topscreen::core::kCursorCatalog)
		code.insert(entry.label_key);
	/* Hotkey descriptions live in the action table, not inside an
	 * obs_module_text("...") call. */
	for (int i = 0; i < topscreen::kHotkeyCount; ++i)
		code.insert(topscreen::kHotkeyLabelKeys[i]);

	CHECK(!code.empty());
	CHECK(!en.empty());

	for (const auto &k : en) {
		++tst::checks();
		if (code.find(k) == code.end())
			tst::fail(__FILE__, __LINE__, "key in en-US.ini is not used by the code: " + k);
	}
}

/* A label must be a label, not an empty string or a copy of its raw key.
 * This catches the classic "obs_module_text missing" symptom before OBS
 * shows an unreadable property name. */
TEST(locale_labels_are_human_readable)
{
	const auto values = ini_values(std::string(TOPSCREEN_TEST_DATA_DIR) + "/locale/en-US.ini");
	CHECK(!values.empty());

	for (const auto &[key, value] : values) {
		++tst::checks();
		if (value.empty()) {
			tst::fail(__FILE__, __LINE__, "empty label for key: " + key);
			continue;
		}
		++tst::checks();
		if (value == key)
			tst::fail(__FILE__, __LINE__, "label is the raw key: " + key);
		++tst::checks();
		if (value.front() == ' ' || value.front() == '\t' || value.back() == ' ' ||
		    value.back() == '\t')
			tst::fail(__FILE__, __LINE__, "label has leading/trailing whitespace: " + key);
	}
}

/* The Brazilian Portuguese translation must cover exactly the same keys as
 * en-US and must not contain empty labels. */
TEST(locale_pt_br_mirrors_en_us)
{
	const std::string en_path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/locale/en-US.ini";
	const std::string pt_path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/locale/pt-BR.ini";
	const auto en = ini_keys(en_path);
	const auto pt = ini_keys(pt_path);
	const auto pt_values = ini_values(pt_path);
	REQUIRE(!en.empty());
	REQUIRE(!pt.empty());

	for (const auto &key : en) {
		++tst::checks();
		if (pt.find(key) == pt.end())
			tst::fail(__FILE__, __LINE__, "key missing from pt-BR.ini: " + key);
	}
	for (const auto &key : pt) {
		++tst::checks();
		if (en.find(key) == en.end())
			tst::fail(__FILE__, __LINE__, "key in pt-BR.ini is not in en-US.ini: " + key);
		++tst::checks();
		const auto it = pt_values.find(key);
		if (it == pt_values.end() || it->second.empty())
			tst::fail(__FILE__, __LINE__, "empty pt-BR label for key: " + key);
	}
}
