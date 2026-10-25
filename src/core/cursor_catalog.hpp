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
	Custom,
	Count,
};

/**
 * Cursor list shown in the filter properties.
 *
 * The array order IS the UI order: every cursor of a theme stays together
 * (all Bibata first, then all Breeze, ...), so the dropdown is easy to scan.
 * The `CursorStyle` enum values never change with this ordering - existing
 * profiles keep pointing at the same bitmap. Removed entries stay in the enum
 * and atlas as retired IDs; they are absent from this menu list.
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

	/* ---- Breeze (regular, Snow; arrow, link hand) ---- */
	{static_cast<int>(CursorStyle::Breeze), CursorFamily::Breeze, "Cursor.Style.Breeze"},
	{static_cast<int>(CursorStyle::BreezeHover), CursorFamily::Breeze, "Cursor.Style.BreezeHover"},
	{static_cast<int>(CursorStyle::BreezeSnow), CursorFamily::Breeze, "Cursor.Style.BreezeSnow"},
	{static_cast<int>(CursorStyle::BreezeSnowHover), CursorFamily::Breeze,
	 "Cursor.Style.BreezeSnowHover"},

	/* ---- Adwaita (arrow, link hand) ---- */
	{static_cast<int>(CursorStyle::Adwaita), CursorFamily::Adwaita, "Cursor.Style.Adwaita"},
	{static_cast<int>(CursorStyle::AdwaitaHover), CursorFamily::Adwaita, "Cursor.Style.AdwaitaHover"},

	/* ---- Yaru (arrow, link hand) ---- */
	{static_cast<int>(CursorStyle::Yaru), CursorFamily::Yaru, "Cursor.Style.Yaru"},
	{static_cast<int>(CursorStyle::YaruHover), CursorFamily::Yaru, "Cursor.Style.YaruHover"},

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

	/* ---- User-supplied custom cursors (Resources/cursors) ---- */
	{static_cast<int>(CursorStyle::CustomPurpleIris), CursorFamily::Custom, "Cursor.Style.CustomPurpleIris"},
	{static_cast<int>(CursorStyle::CustomPurpleIrisLink), CursorFamily::Custom, "Cursor.Style.CustomPurpleIrisLink"},
	{static_cast<int>(CursorStyle::CustomStrawberryMilk), CursorFamily::Custom, "Cursor.Style.CustomStrawberryMilk"},
	{static_cast<int>(CursorStyle::CustomStrawberryMilkLink), CursorFamily::Custom, "Cursor.Style.CustomStrawberryMilkLink"},
	{static_cast<int>(CursorStyle::CustomInterferenceLink), CursorFamily::Custom, "Cursor.Style.CustomInterferenceLink"},
	{static_cast<int>(CursorStyle::CustomInterference), CursorFamily::Custom, "Cursor.Style.CustomInterference"},
	{static_cast<int>(CursorStyle::CustomChromaticAlt), CursorFamily::Custom, "Cursor.Style.CustomChromaticAlt"},
	{static_cast<int>(CursorStyle::CustomChromatic), CursorFamily::Custom, "Cursor.Style.CustomChromatic"},
	{static_cast<int>(CursorStyle::CustomNeonHand2769), CursorFamily::Custom, "Cursor.Style.CustomNeonHand2769"},
	{static_cast<int>(CursorStyle::CustomCrystal2769), CursorFamily::Custom, "Cursor.Style.CustomCrystal2769"},
	{static_cast<int>(CursorStyle::CustomWhiteHand2786), CursorFamily::Custom, "Cursor.Style.CustomWhiteHand2786"},
	{static_cast<int>(CursorStyle::CustomWhiteArrow2786), CursorFamily::Custom, "Cursor.Style.CustomWhiteArrow2786"},
	{static_cast<int>(CursorStyle::CustomWireframeAlt), CursorFamily::Custom, "Cursor.Style.CustomWireframeAlt"},
	{static_cast<int>(CursorStyle::CustomWireframe), CursorFamily::Custom, "Cursor.Style.CustomWireframe"},
	{static_cast<int>(CursorStyle::CustomAmethyst2898), CursorFamily::Custom, "Cursor.Style.CustomAmethyst2898"},
	{static_cast<int>(CursorStyle::CustomSapphire2898), CursorFamily::Custom, "Cursor.Style.CustomSapphire2898"},
	{static_cast<int>(CursorStyle::CustomLavenderClassic), CursorFamily::Custom, "Cursor.Style.CustomLavenderClassic"},
	{static_cast<int>(CursorStyle::CustomLavenderClassicLink), CursorFamily::Custom, "Cursor.Style.CustomLavenderClassicLink"},
	{static_cast<int>(CursorStyle::CustomCheckerboard), CursorFamily::Custom, "Cursor.Style.CustomCheckerboard"},
	{static_cast<int>(CursorStyle::CustomCheckerboardLink), CursorFamily::Custom, "Cursor.Style.CustomCheckerboardLink"},
	{static_cast<int>(CursorStyle::CustomEnergyLink), CursorFamily::Custom, "Cursor.Style.CustomEnergyLink"},
	{static_cast<int>(CursorStyle::CustomSilver), CursorFamily::Custom, "Cursor.Style.CustomSilver"},
	{static_cast<int>(CursorStyle::CustomSilverLink), CursorFamily::Custom, "Cursor.Style.CustomSilverLink"},
	{static_cast<int>(CursorStyle::CustomPixelSword), CursorFamily::Custom, "Cursor.Style.CustomPixelSword"},
	{static_cast<int>(CursorStyle::CustomAxolotl), CursorFamily::Custom, "Cursor.Style.CustomAxolotl"},
	{static_cast<int>(CursorStyle::CustomJester), CursorFamily::Custom, "Cursor.Style.CustomJester"},
	{static_cast<int>(CursorStyle::CustomWitchHat), CursorFamily::Custom, "Cursor.Style.CustomWitchHat"},
	{static_cast<int>(CursorStyle::CustomEnchantedSword), CursorFamily::Custom, "Cursor.Style.CustomEnchantedSword"},
	{static_cast<int>(CursorStyle::CustomPixel3d), CursorFamily::Custom, "Cursor.Style.CustomPixel3d"},
	{static_cast<int>(CursorStyle::CustomPixel3dLink), CursorFamily::Custom, "Cursor.Style.CustomPixel3dLink"},
	{static_cast<int>(CursorStyle::CustomPrismLink1237), CursorFamily::Custom, "Cursor.Style.CustomPrismLink1237"},
	{static_cast<int>(CursorStyle::CustomPrism1238), CursorFamily::Custom, "Cursor.Style.CustomPrism1238"},
	{static_cast<int>(CursorStyle::CustomVioletLink1287), CursorFamily::Custom, "Cursor.Style.CustomVioletLink1287"},
	{static_cast<int>(CursorStyle::CustomViolet1288), CursorFamily::Custom, "Cursor.Style.CustomViolet1288"},
	{static_cast<int>(CursorStyle::CustomShadowLink1359), CursorFamily::Custom, "Cursor.Style.CustomShadowLink1359"},
	{static_cast<int>(CursorStyle::CustomShadow1360), CursorFamily::Custom, "Cursor.Style.CustomShadow1360"},
	{static_cast<int>(CursorStyle::CustomPastelLink1717), CursorFamily::Custom, "Cursor.Style.CustomPastelLink1717"},
	{static_cast<int>(CursorStyle::CustomPastel1718), CursorFamily::Custom, "Cursor.Style.CustomPastel1718"},
	{static_cast<int>(CursorStyle::CustomMintLink1823), CursorFamily::Custom, "Cursor.Style.CustomMintLink1823"},
	{static_cast<int>(CursorStyle::CustomLavender1824), CursorFamily::Custom, "Cursor.Style.CustomLavender1824"},
	{static_cast<int>(CursorStyle::CustomCartoonLink58), CursorFamily::Custom, "Cursor.Style.CustomCartoonLink58"},
	{static_cast<int>(CursorStyle::CustomNeon3d), CursorFamily::Custom, "Cursor.Style.CustomNeon3d"},
	{static_cast<int>(CursorStyle::CustomNeon3dLink), CursorFamily::Custom, "Cursor.Style.CustomNeon3dLink"},
	{static_cast<int>(CursorStyle::CustomOctopus), CursorFamily::Custom, "Cursor.Style.CustomOctopus"},
	{static_cast<int>(CursorStyle::CustomOctopusLink), CursorFamily::Custom, "Cursor.Style.CustomOctopusLink"},
	{static_cast<int>(CursorStyle::CustomConstellation), CursorFamily::Custom, "Cursor.Style.CustomConstellation"},
	{static_cast<int>(CursorStyle::CustomComic), CursorFamily::Custom, "Cursor.Style.CustomComic"},
	{static_cast<int>(CursorStyle::CustomStroke), CursorFamily::Custom, "Cursor.Style.CustomStroke"},
	{static_cast<int>(CursorStyle::CustomStrokeLink), CursorFamily::Custom, "Cursor.Style.CustomStrokeLink"},
	{static_cast<int>(CursorStyle::CustomCartoon), CursorFamily::Custom, "Cursor.Style.CustomCartoon"},
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

/** IDs removed from the menu but kept in the atlas for stable profiles. */
inline constexpr int kRetiredCursorStyles[] = {
	6,   /* Adwaita - Hand */
	14,  /* Effects - Crosshair */
	16,  /* Adwaita - Grab hand (open) */
	17,  /* Adwaita - Grab hand (closed) */
	18,  /* Adwaita - I-beam */
	23,  /* Yaru - Grab hand */
	24,  /* Yaru - I-beam */
	25,  /* Breeze - I-beam */
	65,  /* Custom - Paper Boat */
	66,  /* Custom - Paper Plane */
	79,  /* Custom - Bow 41 */
	80,  /* Custom - Dagger 42 */
	81,  /* Custom - Victory Link 57 */
	88,  /* Custom - Constellation Link */
	90,  /* Custom - Comic Link */
	91,  /* Custom - Sketch */
	92,  /* Custom - Sketch Link */
	93,  /* Custom - Diamond */
	94,  /* Custom - Fire */
	95,  /* Custom - Fire Link */
	96,  /* Custom - Glitch */
	97,  /* Custom - Glitch Link */
	98,  /* Custom - Projection */
	99,  /* Custom - Projection Link */
	103, /* Custom - Thumbs Up */
	104, /* Custom - Ribbon */
	105, /* Custom - Ribbon Link */
};

inline bool cursor_style_is_retired(int style)
{
	for (int retired : kRetiredCursorStyles) {
		if (style == retired)
			return true;
	}
	return false;
}

/**
 * Maps a retired style to a sensible replacement:
 * themed arrows for retired built-ins, the first custom arrow for retired PNGs.
 */
inline int cursor_style_fallback(int style)
{
	switch (style) {
	case 6:  /* Adwaita - Hand */
	case 16: /* Adwaita - Grab hand (open) */
	case 17: /* Adwaita - Grab hand (closed) */
	case 18: /* Adwaita - I-beam */
		return static_cast<int>(CursorStyle::Adwaita);
	case 25: /* Breeze - I-beam */
		return static_cast<int>(CursorStyle::Breeze);
	case 23: /* Yaru - Grab hand */
	case 24: /* Yaru - I-beam */
		return static_cast<int>(CursorStyle::Yaru);
	case 14: /* Effects - Crosshair */
		return static_cast<int>(CursorStyle::Dot);
	default:
		return static_cast<int>(CursorStyle::CustomPurpleIris);
	}
}

/** Clamps and replaces a retired style so an old profile never draws a blank cell. */
inline int cursor_style_resolve(int style)
{
	style = cursor_style_clamp(style);
	return cursor_style_is_retired(style) ? cursor_style_fallback(style) : style;
}

/**
 * Whether this style keeps the fixed 30° clockwise image tilt.
 *
 * Only the user-supplied "Custom" cursors keep it. The bundled cursors from
 * Bibata through Dot (including the platform and procedural ones) are drawn
 * upright, as their artwork was designed.
 */
inline bool cursor_style_keeps_tilt(int style)
{
	style = cursor_style_resolve(style);
	return style >= static_cast<int>(CursorStyle::CustomPurpleIris) &&
	       style < static_cast<int>(CursorStyle::Count);
}

} // namespace topscreen::core
