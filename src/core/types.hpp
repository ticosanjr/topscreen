#pragma once

#include <cstdint>

/**
 * Core shared types and enums (independent of OBS).
 *
 * The integer values of the enums are stable: they are persisted in obs_data
 * and used in preset files. Do not reorder without migrating.
 */
namespace topscreen::core {

struct Rgba {
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 1.0f;
};

struct Vec2f {
	double x = 0.0;
	double y = 0.0;
};

enum class BgMode : int {
	Solid = 0,
	Gradient = 1,
	Wallpaper = 2,
	LocalImage = 3,
	Transparent = 4,

	/* Legacy aliases: old configs/presets used 1=V vertical, 2=diagonal and
	 * 3=image. They are migrated to the new types on load. */
	Vertical = Gradient,
	Diagonal = Gradient,
	Image = Wallpaper,
};

enum class CursorStyle : int {
	MacOs = 0,
	Windows = 1,
	Breeze = 2,
	Bibata = 3,
	Adwaita = 4,
	GoogleDot = 5,
	Pointer = 6,
	Dmz = 7,
	Yaru = 8,
	XCursorPro = 9,
	BibataIce = 10,
	BibataOriginal = 11,
	Neon = 12,
	Dot = 13,
	Cross = 14,
	Onyx = 15,
	HandGrab = 16,
	HandGrabbing = 17,
	IBeam = 18,
	DmzBlack = 19,
	GoogleDotBlack = 20,
	BreezeSnow = 21,
	BibataIceOriginal = 22,
	YaruGrab = 23,
	YaruIBeam = 24,
	BreezeIBeam = 25,
	/* Hover / link hands: the very same themes' real `pointer` cursor, so the
	 * "clicking a link" look matches the arrow you picked. Appended (never
	 * inserted) to keep saved profiles pointing at the same style. */
	BreezeHover = 26,
	BibataHover = 27,
	AdwaitaHover = 28,
	DmzHover = 29,
	YaruHover = 30,
	XCursorProHover = 31,
	BibataIceHover = 32,
	BibataOriginalHover = 33,
	DmzBlackHover = 34,
	BreezeSnowHover = 35,
	BibataIceOriginalHover = 36,

	/* User-supplied custom cursors (appended to keep existing profiles). */
	CustomPurpleIris = 37,
	CustomPurpleIrisLink = 38,
	CustomStrawberryMilk = 39,
	CustomStrawberryMilkLink = 40,
	CustomInterferenceLink = 41,
	CustomInterference = 42,
	CustomChromaticAlt = 43,
	CustomChromatic = 44,
	CustomNeonHand2769 = 45,
	CustomCrystal2769 = 46,
	CustomWhiteHand2786 = 47,
	CustomWhiteArrow2786 = 48,
	CustomWireframeAlt = 49,
	CustomWireframe = 50,
	CustomAmethyst2898 = 51,
	CustomSapphire2898 = 52,
	CustomLavenderClassic = 53,
	CustomLavenderClassicLink = 54,
	CustomCheckerboard = 55,
	CustomCheckerboardLink = 56,
	CustomEnergyLink = 57,
	CustomSilver = 58,
	CustomSilverLink = 59,
	CustomPixelSword = 60,
	CustomAxolotl = 61,
	CustomJester = 62,
	CustomWitchHat = 63,
	CustomEnchantedSword = 64,
	CustomPaperBoat = 65,
	CustomPaperPlane = 66,
	CustomPixel3d = 67,
	CustomPixel3dLink = 68,
	CustomPrismLink1237 = 69,
	CustomPrism1238 = 70,
	CustomVioletLink1287 = 71,
	CustomViolet1288 = 72,
	CustomShadowLink1359 = 73,
	CustomShadow1360 = 74,
	CustomPastelLink1717 = 75,
	CustomPastel1718 = 76,
	CustomMintLink1823 = 77,
	CustomLavender1824 = 78,
	CustomBow41 = 79,
	CustomDagger42 = 80,
	CustomVictoryLink57 = 81,
	CustomCartoonLink58 = 82,
	CustomNeon3d = 83,
	CustomNeon3dLink = 84,
	CustomOctopus = 85,
	CustomOctopusLink = 86,
	CustomConstellation = 87,
	CustomConstellationLink = 88,
	CustomComic = 89,
	CustomComicLink = 90,
	CustomSketch = 91,
	CustomSketchLink = 92,
	CustomDiamond = 93,
	CustomFire = 94,
	CustomFireLink = 95,
	CustomGlitch = 96,
	CustomGlitchLink = 97,
	CustomProjection = 98,
	CustomProjectionLink = 99,
	CustomStroke = 100,
	CustomStrokeLink = 101,
	CustomCartoon = 102,
	CustomThumbsUp = 103,
	CustomRibbon = 104,
	CustomRibbonLink = 105,
	Count = 106,
};

/** Click effect types. */
enum class ClickEffect : int {
	Ripple = 0,
	Orb = 1,
	Ring = 2,
	Force = 3,
	Glow = 4,
	Echo = 5,
	Flare = 6,
	QuietCircle = 7,
	WaterRipple = 8,
	WaterRippleClear = 9,
};

enum class ZoomMode : int {
	Off = 0,
	Auto = 1,
	Manual = 2,
};

} // namespace topscreen::core
