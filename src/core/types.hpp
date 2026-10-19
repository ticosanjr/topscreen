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
	Vertical = 1,
	Diagonal = 2,
	Image = 3,
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
	Count = 37,
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
};

enum class ZoomMode : int {
	Off = 0,
	Auto = 1,
	Manual = 2,
};

} // namespace topscreen::core
