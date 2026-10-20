#include "hotkeys/ts-hotkey-actions.hpp"

#include "test_framework.hpp"

#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_SOURCE_DIR
#define TOPSCREEN_SOURCE_DIR "."
#endif

using namespace topscreen;

namespace {

std::string read_text(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

std::string locale_value(const std::string &ini, const std::string &key)
{
	const std::string needle = key + "=";
	auto pos = ini.find("\n" + needle);
	if (pos == std::string::npos) {
		if (ini.rfind(needle, 0) != 0)
			return "";
		pos = 0;
	} else {
		++pos; /* skip the newline */
	}
	const auto eol = ini.find('\n', pos);
	std::string value = ini.substr(pos + needle.size(),
				       (eol == std::string::npos ? ini.size() : eol) -
					       (pos + needle.size()));
	if (!value.empty() && value.back() == '\r')
		value.pop_back();
	return value;
}

} // namespace

/* Every registered ID must be findable by the same prefix in OBS. */
TEST(hotkey_ids_all_start_with_the_prefix)
{
	REQUIRE(kHotkeyCount == HK_Count);
	const size_t prefix_len = std::strlen(kHotkeyPrefix);
	for (int i = 0; i < kHotkeyCount; ++i) {
		REQUIRE(kHotkeyNames[i] != nullptr);
		++tst::checks();
		if (std::strncmp(kHotkeyNames[i], kHotkeyPrefix, prefix_len) != 0)
			tst::fail(__FILE__, __LINE__,
				  std::string("hotkey id without TopScreen_ prefix: ") + kHotkeyNames[i]);
	}
}

TEST(hotkey_ids_are_unique)
{
	std::set<std::string> ids;
	for (int i = 0; i < kHotkeyCount; ++i) {
		++tst::checks();
		if (!ids.insert(kHotkeyNames[i]).second)
			tst::fail(__FILE__, __LINE__,
				  std::string("duplicate hotkey id: ") + kHotkeyNames[i]);
	}
}

/* The label OBS shows must contain the literal ID, so typing "TopScreen_"
 * in Settings -> Hotkeys finds every action (including future ones). */
TEST(hotkey_descriptions_start_with_the_literal_id)
{
	const std::string ini = read_text(std::string(TOPSCREEN_SOURCE_DIR) + "/data/locale/en-US.ini");
	REQUIRE(!ini.empty());

	const std::string prefix = kHotkeyPrefix;
	for (int i = 0; i < kHotkeyCount; ++i) {
		const std::string id = kHotkeyNames[i];
		REQUIRE(id.rfind(prefix, 0) == 0);
		const std::string action = id.substr(prefix.size());
		const std::string value = locale_value(ini, "Hotkey." + action);

		++tst::checks();
		if (value.empty())
			tst::fail(__FILE__, __LINE__,
				  std::string("missing hotkey description: Hotkey.") + action);
		else if (value.rfind(id, 0) != 0)
			tst::fail(__FILE__, __LINE__,
				  std::string("hotkey description must start with the id: ") + id);
	}
}
