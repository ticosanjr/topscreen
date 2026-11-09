#include "core/cursor_catalog.hpp"

#include "test_framework.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_SOURCE_DIR
#define TOPSCREEN_SOURCE_DIR "."
#endif

using namespace topscreen::core;

namespace {

std::string read_text(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

} // namespace

TEST(cursor_catalog_covers_every_active_style_exactly_once)
{
	CHECK_EQ(kCursorCatalogCount, 79);

	bool seen[static_cast<int>(CursorStyle::Count)] = {};
	for (const auto &entry : kCursorCatalog) {
		REQUIRE(entry.style >= 0);
		REQUIRE(entry.style < static_cast<int>(CursorStyle::Count));
		++tst::checks();
		if (cursor_style_is_retired(entry.style))
			tst::fail(__FILE__, __LINE__, "retired cursor style is still in the menu");
		if (seen[entry.style])
			tst::fail(__FILE__, __LINE__, "duplicate cursor style in the catalog");
		seen[entry.style] = true;
	}
	for (int i = 0; i < static_cast<int>(CursorStyle::Count); ++i) {
		if (cursor_style_is_retired(i))
			continue;
		++tst::checks();
		if (!seen[i])
			tst::fail(__FILE__, __LINE__, "active cursor style missing from the catalog");
	}
}

/* The whole point of the catalog order: every theme is one contiguous block,
 * so all Bibata cursors sit together in the dropdown. */
TEST(cursor_catalog_keeps_families_together_and_ordered)
{
	std::set<int> seen;
	int prev = -1;
	for (const auto &entry : kCursorCatalog) {
		const int fam = static_cast<int>(entry.family);
		if (fam != prev) {
			++tst::checks();
			if (seen.count(fam))
				tst::fail(__FILE__, __LINE__, "cursor family is not contiguous");
			seen.insert(fam);
			++tst::checks();
			if (fam < prev)
				tst::fail(__FILE__, __LINE__, "cursor families are out of order");
			prev = fam;
		}
	}
}

TEST(cursor_catalog_labels_are_unique_and_present_in_the_locale)
{
	const std::string ini =
		read_text(std::string(TOPSCREEN_SOURCE_DIR) + "/data/locale/en-US.ini");
	REQUIRE(!ini.empty());

	std::set<std::string> labels;
	for (const auto &entry : kCursorCatalog) {
		REQUIRE(entry.label_key != nullptr);
		++tst::checks();
		if (!labels.insert(entry.label_key).second)
			tst::fail(__FILE__, __LINE__,
				  std::string("duplicate cursor label key: ") + entry.label_key);

		const std::string needle = std::string(entry.label_key) + "=";
		const auto pos = ini.find(needle);
		++tst::checks();
		if (pos == std::string::npos) {
			tst::fail(__FILE__, __LINE__,
				  std::string("cursor label missing from en-US.ini: ") + entry.label_key);
		} else {
			const auto eol = ini.find('\n', pos);
			std::string value = ini.substr(pos + needle.size(),
						       (eol == std::string::npos ? ini.size() : eol) -
							       (pos + needle.size()));
			if (!value.empty() && value.back() == '\r')
				value.pop_back();
			++tst::checks();
			if (value.empty()) {
				tst::fail(__FILE__, __LINE__,
					  std::string("empty cursor label: ") + entry.label_key);
			} else {
				std::string lower = value;
				std::transform(lower.begin(), lower.end(), lower.begin(),
					       [](unsigned char c) {
						       return static_cast<char>(std::tolower(c));
					       });
				++tst::checks();
				if (lower.find("starter") != std::string::npos ||
				    lower.find("pointer") != std::string::npos ||
				    lower.find("mixkit") != std::string::npos)
					tst::fail(__FILE__, __LINE__,
						  std::string("forbidden word in cursor label: ") + value);
			}
		}
	}
}

TEST(cursor_catalog_starts_with_the_bibata_family)
{
	REQUIRE(kCursorCatalogCount > 0);
	CHECK_EQ(static_cast<int>(kCursorCatalog[0].family), static_cast<int>(CursorFamily::Bibata));
	/* All Bibata entries come first (8: four themes x arrow + link hand). */
	int bibata = 0;
	for (const auto &entry : kCursorCatalog) {
		if (entry.family != CursorFamily::Bibata)
			break;
		++bibata;
	}
	CHECK_EQ(bibata, 8);
}

TEST(cursor_style_clamp_handles_invalid_values)
{
	CHECK_EQ(cursor_style_clamp(-1), 0);
	CHECK_EQ(cursor_style_clamp(0), 0);
	CHECK_EQ(cursor_style_clamp(static_cast<int>(CursorStyle::Bibata)),
		 static_cast<int>(CursorStyle::Bibata));
	CHECK_EQ(cursor_style_clamp(static_cast<int>(CursorStyle::Count)),
		 static_cast<int>(CursorStyle::Count) - 1);
	CHECK_EQ(cursor_style_clamp(999), static_cast<int>(CursorStyle::Count) - 1);
}

TEST(cursor_style_resolve_retires_old_profiles_safely)
{
	CHECK(cursor_style_is_retired(static_cast<int>(CursorStyle::BreezeIBeam)));
	CHECK(cursor_style_is_retired(static_cast<int>(CursorStyle::CustomRibbonLink)));
	CHECK(!cursor_style_is_retired(static_cast<int>(CursorStyle::CustomStroke)));
	CHECK_EQ(cursor_style_resolve(static_cast<int>(CursorStyle::BreezeIBeam)),
		 static_cast<int>(CursorStyle::Breeze));
	CHECK_EQ(cursor_style_resolve(static_cast<int>(CursorStyle::YaruIBeam)),
		 static_cast<int>(CursorStyle::Yaru));
	CHECK_EQ(cursor_style_resolve(static_cast<int>(CursorStyle::Cross)),
		 static_cast<int>(CursorStyle::Dot));
	CHECK_EQ(cursor_style_resolve(static_cast<int>(CursorStyle::CustomRibbonLink)),
		 static_cast<int>(CursorStyle::CustomPurpleIris));
	CHECK_EQ(cursor_style_resolve(999), static_cast<int>(CursorStyle::CustomPurpleIris));
}

TEST(cursor_retired_styles_are_valid_and_have_active_fallbacks)
{
	std::set<int> retired;
	for (int style : kRetiredCursorStyles) {
		REQUIRE(style >= 0);
		REQUIRE(style < static_cast<int>(CursorStyle::Count));
		++tst::checks();
		if (!retired.insert(style).second)
			tst::fail(__FILE__, __LINE__, "duplicate retired cursor style");

		++tst::checks();
		for (const auto &entry : kCursorCatalog) {
			if (entry.style == style) {
				tst::fail(__FILE__, __LINE__,
					  "retired cursor style is still in the catalog");
				break;
			}
		}

		const int fallback = cursor_style_fallback(style);
		++tst::checks();
		if (fallback < 0 || fallback >= static_cast<int>(CursorStyle::Count))
			tst::fail(__FILE__, __LINE__, "cursor fallback is out of range");
		++tst::checks();
		if (cursor_style_is_retired(fallback))
			tst::fail(__FILE__, __LINE__, "cursor fallback is itself retired");
		++tst::checks();
		if (cursor_style_resolve(style) != fallback)
			tst::fail(__FILE__, __LINE__, "retired style does not resolve to its fallback");
	}
}
