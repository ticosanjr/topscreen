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
	Count = 16,
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
