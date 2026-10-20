#pragma once

#include "types.hpp"

namespace topscreen::core {

/** Theme a cursor belongs to. Used to keep the same family together in the UI. */
enum class CursorFamily : int {
	Bibata = 0,
	Breeze,
	Adwaita,
	Yaru,
	Dmz,
	GoogleDot,
	XCursorPro,
	MacOs,
	Windows,
	Effects,
	Count,
};

/**
 * Cursor list shown in the filter properties.
 *
 * The array order IS the UI order: every cursor of a theme stays together
 * (all Bibata first, then all Breeze, ...), so the dropdown is easy to scan.
 * The `CursorStyle` enum values never change with this ordering - existing
 * profiles keep pointing at the same bitmap.
 */
struct CursorCatalogEntry {
	int style;
	CursorFamily family;
	const char *label_key;
};

inline constexpr CursorCatalogEntry kCursorCatalog[] = {
	/* ---- Bibata (Classic, Ice, Original, Ice Original; arrow + link hand) ---- */
	{static_cast<int>(CursorStyle::Bibata), CursorFamily::Bibata, "Cursor.Style.Bibata"},
	{static_cast<int>(CursorStyle::BibataHover), CursorFamily::Bibata, "Cursor.Style.BibataHover"},
	{static_cast<int>(CursorStyle::BibataIce), CursorFamily::Bibata, "Cursor.Style.BibataIce"},
	{static_cast<int>(CursorStyle::BibataIceHover), CursorFamily::Bibata, "Cursor.Style.BibataIceHover"},
	{static_cast<int>(CursorStyle::BibataOriginal), CursorFamily::Bibata,
	 "Cursor.Style.BibataOriginal"},
	{static_cast<int>(CursorStyle::BibataOriginalHover), CursorFamily::Bibata,
	 "Cursor.Style.BibataOriginalHover"},
	{static_cast<int>(CursorStyle::BibataIceOriginal), CursorFamily::Bibata,
	 "Cursor.Style.BibataIceOriginal"},
	{static_cast<int>(CursorStyle::BibataIceOriginalHover), CursorFamily::Bibata,
	 "Cursor.Style.BibataIceOriginalHover"},

	/* ---- Breeze (regular, Snow; arrow, link hand, I-beam) ---- */
	{static_cast<int>(CursorStyle::Breeze), CursorFamily::Breeze, "Cursor.Style.Breeze"},
	{static_cast<int>(CursorStyle::BreezeHover), CursorFamily::Breeze, "Cursor.Style.BreezeHover"},
	{static_cast<int>(CursorStyle::BreezeSnow), CursorFamily::Breeze, "Cursor.Style.BreezeSnow"},
	{static_cast<int>(CursorStyle::BreezeSnowHover), CursorFamily::Breeze,
	 "Cursor.Style.BreezeSnowHover"},
	{static_cast<int>(CursorStyle::BreezeIBeam), CursorFamily::Breeze, "Cursor.Style.BreezeIBeam"},

	/* ---- Adwaita (arrow, link hand, plus the generic hands/text shipped by it) ---- */
	{static_cast<int>(CursorStyle::Adwaita), CursorFamily::Adwaita, "Cursor.Style.Adwaita"},
	{static_cast<int>(CursorStyle::AdwaitaHover), CursorFamily::Adwaita, "Cursor.Style.AdwaitaHover"},
	{static_cast<int>(CursorStyle::Pointer), CursorFamily::Adwaita, "Cursor.Style.Pointer"},
	{static_cast<int>(CursorStyle::HandGrab), CursorFamily::Adwaita, "Cursor.Style.HandGrab"},
	{static_cast<int>(CursorStyle::HandGrabbing), CursorFamily::Adwaita,
	 "Cursor.Style.HandGrabbing"},
	{static_cast<int>(CursorStyle::IBeam), CursorFamily::Adwaita, "Cursor.Style.IBeam"},

	/* ---- Yaru (arrow, link hand, grab hand, I-beam) ---- */
	{static_cast<int>(CursorStyle::Yaru), CursorFamily::Yaru, "Cursor.Style.Yaru"},
	{static_cast<int>(CursorStyle::YaruHover), CursorFamily::Yaru, "Cursor.Style.YaruHover"},
	{static_cast<int>(CursorStyle::YaruGrab), CursorFamily::Yaru, "Cursor.Style.YaruGrab"},
	{static_cast<int>(CursorStyle::YaruIBeam), CursorFamily::Yaru, "Cursor.Style.YaruIBeam"},

	/* ---- DMZ (White and Black; arrow + link hand) ---- */
	{static_cast<int>(CursorStyle::Dmz), CursorFamily::Dmz, "Cursor.Style.Dmz"},
	{static_cast<int>(CursorStyle::DmzHover), CursorFamily::Dmz, "Cursor.Style.DmzHover"},
	{static_cast<int>(CursorStyle::DmzBlack), CursorFamily::Dmz, "Cursor.Style.DmzBlack"},
	{static_cast<int>(CursorStyle::DmzBlackHover), CursorFamily::Dmz, "Cursor.Style.DmzBlackHover"},

	/* ---- Google Dot (White and Black) ---- */
	{static_cast<int>(CursorStyle::GoogleDot), CursorFamily::GoogleDot, "Cursor.Style.GoogleDot"},
	{static_cast<int>(CursorStyle::GoogleDotBlack), CursorFamily::GoogleDot,
	 "Cursor.Style.GoogleDotBlack"},

	/* ---- XCursor Pro (arrow + link hand) ---- */
	{static_cast<int>(CursorStyle::XCursorPro), CursorFamily::XCursorPro, "Cursor.Style.XCursorPro"},
	{static_cast<int>(CursorStyle::XCursorProHover), CursorFamily::XCursorPro,
	 "Cursor.Style.XCursorProHover"},

	/* ---- Classic platform cursors ---- */
	{static_cast<int>(CursorStyle::MacOs), CursorFamily::MacOs, "Cursor.Style.MacOs"},
	{static_cast<int>(CursorStyle::Windows), CursorFamily::Windows, "Cursor.Style.Windows"},

	/* ---- Procedural effect cursors ---- */
	{static_cast<int>(CursorStyle::Neon), CursorFamily::Effects, "Cursor.Style.Neon"},
	{static_cast<int>(CursorStyle::Onyx), CursorFamily::Effects, "Cursor.Style.Onyx"},
	{static_cast<int>(CursorStyle::Dot), CursorFamily::Effects, "Cursor.Style.Dot"},
	{static_cast<int>(CursorStyle::Cross), CursorFamily::Effects, "Cursor.Style.Cross"},
};

inline constexpr int kCursorCatalogCount =
	static_cast<int>(sizeof(kCursorCatalog) / sizeof(kCursorCatalog[0]));

/** Keeps a persisted style inside the atlas range (old/invalid values). */
inline int cursor_style_clamp(int style)
{
	const int count = static_cast<int>(CursorStyle::Count);
	if (style < 0)
		return 0;
	if (style >= count)
		return count - 1;
	return style;
}

} // namespace topscreen::core
