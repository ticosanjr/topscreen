#pragma once

/*
 * Single source for UI slider ranges and config sanitization limits.
 *
 * The Properties UI and sanitize_config() must agree: a slider must never
 * offer a value that sanitization clamps, and sanitization must not accept
 * values the UI cannot reach. Keep the range definitions here.
 */

namespace topscreen::core::limits {

struct Range {
	double min;
	double max;
	double step;
};

struct IntRange {
	int min;
	int max;
	int step;
};

/** 0..1 with a 1% step (opacity, positions, blends). */
inline constexpr Range kUnitRange{0.0, 1.0, 0.01};

inline constexpr Range kBgGradientAngleRange{0.0, 360.0, 1.0};
inline constexpr Range kBgImageBlurRange{0.0, 60.0, 1.0};
inline constexpr Range kBgPaddingRange{0.0, 400.0, 1.0};
inline constexpr Range kCornerRadiusRange{0.0, 200.0, 1.0};
inline constexpr Range kShadowBlurRange{0.0, 200.0, 1.0};
inline constexpr Range kShadowOffsetRange{-100.0, 100.0, 1.0};

inline constexpr Range kCursorSizeRange{0.5, 4.0, 0.05};
inline constexpr Range kCursorRotationRange{-180.0, 180.0, 1.0};
inline constexpr Range kCursorGlideRange{50.0, 1200.0, 10.0};
inline constexpr Range kCursorIdleDelayRange{0.0, 5000.0, 50.0};

inline constexpr Range kZoomAmountRange{1.0, 4.0, 0.05};
inline constexpr Range kZoomDurationRange{100.0, 2000.0, 10.0};
inline constexpr Range kZoomIdleTimeoutRange{200.0, 10000.0, 50.0};

inline constexpr Range kClickSizeRange{0.1, 4.0, 0.05};
inline constexpr Range kClickDurationRange{100.0, 2000.0, 10.0};

inline constexpr Range kRadiusRange{0.05, 1.0, 0.01};
inline constexpr Range kBorderWidthRange{0.5, 40.0, 0.5};
inline constexpr Range kInOutMsRange{0.0, 2000.0, 10.0};

inline constexpr Range kKeysTtlRange{500.0, 8000.0, 50.0};
inline constexpr IntRange kKeysMaxRange{1, 12, 1};
inline constexpr Range kKeysSizeRange{0.4, 4.0, 0.05};

inline constexpr Range kHighlightThicknessRange{0.001, 0.05, 0.001};
inline constexpr Range kHighlightHoldRange{0.0, 5000.0, 50.0};
inline constexpr Range kHighlightFadeRange{0.0, 2000.0, 50.0};
inline constexpr Range kHighlightZoomMaxRange{1.2, 8.0, 0.1};
inline constexpr Range kHighlightZoomInRange{50.0, 2000.0, 10.0};
inline constexpr Range kHighlightZoomOutRange{100.0, 8000.0, 50.0};

inline constexpr IntRange kMonitorPositionRange{-32768, 32767, 1};

} // namespace topscreen::core::limits
