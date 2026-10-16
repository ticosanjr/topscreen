#pragma once

#include <cstdint>

/**
 * Tipos e enums compartilhados do nucleo (independentes do OBS).
 *
 * Os valores inteiros dos enums sao estaveis: eles sao persistidos em obs_data
 * e usados em arquivos de preset. Nao reordene sem migrar.
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
	Count = 12,
};

/** Tipos de efeito de clique. */
enum class ClickEffect : int {
	Ripple = 0,
	Orb = 1,
	Ring = 2,
	Force = 3,
};

enum class ZoomMode : int {
	Off = 0,
	Auto = 1,
	Manual = 2,
};

} // namespace topscreen::core
