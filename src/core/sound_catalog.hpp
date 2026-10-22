#pragma once

namespace topscreen::core {

/** One bundled click sound shown in the Properties dropdown. */
struct SoundEntry {
	const char *label;
	const char *path;
};

/**
 * Built-in click sounds (data/sounds). The user-supplied pack follows the
 * original realistic/FX palette; every file is mono 16-bit PCM at 44.1 kHz.
 */
inline constexpr SoundEntry kSounds[] = {
	/* real mouse/switch clicks */
	{"Mouse (tight click)", "sounds/click-mouse-tight.wav"},
	{"Mouse (single click)", "sounds/click-mouse-single.wav"},
	{"Mouse (right click)", "sounds/click-mouse-right.wav"},
	{"Retro (double)", "sounds/click-retro.wav"},
	{"Optical (dry)", "sounds/click-optical.wav"},
	{"Muted (quiet)", "sounds/click-muted.wav"},
	/* FX palette */
	{"Soft", "sounds/click-soft.wav"},
	{"Hard", "sounds/click-hard.wav"},
	{"Tick", "sounds/click-tick.wav"},
	{"Bubble", "sounds/click-bubble.wav"},
	{"Thock", "sounds/click-thock.wav"},
	/* user-supplied click pack */
	{"Clear Mouse Clicks", "sounds/clear-mouse-clicks.wav"},
	{"Fast Double Click", "sounds/fast-double-click.wav"},
	{"Gun Click", "sounds/gun-click.wav"},
	{"Handgun Click", "sounds/handgun-click.wav"},
	{"Hard Click", "sounds/hard-click.wav"},
	{"Typewriter", "sounds/typewriter.wav"},
	{"Interface Hover", "sounds/interface-hover.wav"},
	{"Tech Select", "sounds/tech-select.wav"},
	{"Mouse Close", "sounds/mouse-close.wav"},
	{"Mouse Hard", "sounds/mouse-hard.wav"},
	{"Old Camera", "sounds/old-camera.wav"},
	{"Pen Click", "sounds/pen-click.wav"},
	{"Positive Interface", "sounds/positive-interface.wav"},
	{"Single Classic", "sounds/single-classic.wav"},
	{"Stapler", "sounds/stapler.wav"},
};

inline constexpr int kSoundCount = static_cast<int>(sizeof(kSounds) / sizeof(kSounds[0]));

} // namespace topscreen::core
