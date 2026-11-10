#include "core/config.hpp"
#include "core/config_limits.hpp"

#include "test_framework.hpp"

using namespace topscreen::core;

namespace {

void check_range(const limits::Range &r)
{
	CHECK(r.min < r.max);
	CHECK(r.step > 0.0);
}

void check_range(const limits::IntRange &r)
{
	CHECK(r.min < r.max);
	CHECK(r.step > 0);
}

} // namespace

TEST(config_limits_are_sane)
{
	check_range(limits::kUnitRange);
	check_range(limits::kBgGradientAngleRange);
	check_range(limits::kBgImageBlurRange);
	check_range(limits::kBgPaddingRange);
	check_range(limits::kCornerRadiusRange);
	check_range(limits::kShadowBlurRange);
	check_range(limits::kShadowOffsetRange);
	check_range(limits::kFrameWidthRange);
	check_range(limits::kCursorSizeRange);
	check_range(limits::kCursorRotationRange);
	check_range(limits::kCursorGlideRange);
	check_range(limits::kCursorIdleDelayRange);
	check_range(limits::kCursorSmoothRange);
	check_range(limits::kZoomAmountRange);
	check_range(limits::kZoomDurationRange);
	check_range(limits::kZoomIdleTimeoutRange);
	check_range(limits::kZoomSmartHoldRange);
	check_range(limits::kClickSizeRange);
	check_range(limits::kClickDurationRange);
	check_range(limits::kRadiusRange);
	check_range(limits::kBorderWidthRange);
	check_range(limits::kInOutMsRange);
	check_range(limits::kKeysTtlRange);
	check_range(limits::kKeysMaxRange);
	check_range(limits::kKeysSizeRange);
	check_range(limits::kHighlightThicknessRange);
	check_range(limits::kHighlightHoldRange);
	check_range(limits::kHighlightFadeRange);
	check_range(limits::kHighlightZoomMaxRange);
	check_range(limits::kHighlightZoomInRange);
	check_range(limits::kHighlightZoomOutRange);
	check_range(limits::kMonitorPositionRange);
}

/* Every default must be reachable by the matching slider, and sanitization
 * must not move it. Keeping this table next to config_limits.hpp catches drift
 * when a default or a range changes. */
TEST(config_defaults_are_inside_the_shared_ranges)
{
	const Config c;

#define CHECK_DEFAULT(range, field)                                                                \
	do {                                                                                       \
		CHECK(limits::range.min <= c.field);                                               \
		CHECK(c.field <= limits::range.max);                                               \
	} while (0)

	CHECK_DEFAULT(kBgGradientAngleRange, bg.gradient_angle);
	CHECK_DEFAULT(kUnitRange, bg.gradient_center_x);
	CHECK_DEFAULT(kUnitRange, bg.gradient_center_y);
	CHECK_DEFAULT(kBgImageBlurRange, bg.image_blur);
	CHECK_DEFAULT(kBgPaddingRange, bg.padding_x);
	CHECK_DEFAULT(kBgPaddingRange, bg.padding_y);
	CHECK_DEFAULT(kCornerRadiusRange, bg.corner_radius);
	CHECK_DEFAULT(kUnitRange, bg.shadow_alpha);
	CHECK_DEFAULT(kShadowBlurRange, bg.shadow_blur);
	CHECK_DEFAULT(kShadowOffsetRange, bg.shadow_offset_x);
	CHECK_DEFAULT(kShadowOffsetRange, bg.shadow_offset);
	CHECK_DEFAULT(kFrameWidthRange, bg.frame_width);

	CHECK_DEFAULT(kCursorSizeRange, cursor.size);
	CHECK_DEFAULT(kCursorRotationRange, cursor.rotation_offset_deg);
	CHECK_DEFAULT(kCursorGlideRange, cursor.glide_ms);
	CHECK_DEFAULT(kCursorIdleDelayRange, cursor.idle_delay_ms);
	CHECK_DEFAULT(kCursorSmoothRange, cursor.smooth_ms);

	CHECK_DEFAULT(kZoomAmountRange, zoom.amount);
	CHECK_DEFAULT(kZoomDurationRange, zoom.duration_ms);
	CHECK_DEFAULT(kZoomIdleTimeoutRange, zoom.idle_timeout_ms);
	CHECK_DEFAULT(kZoomSmartHoldRange, zoom.smart_hold_ms);
	CHECK_DEFAULT(kUnitRange, zoom.pan_amount);
	CHECK_DEFAULT(kUnitRange, zoom.center_amount);

	CHECK_DEFAULT(kClickSizeRange, click_left.size);
	CHECK_DEFAULT(kClickDurationRange, click_left.duration_ms);
	CHECK_DEFAULT(kClickSizeRange, click_right.size);
	CHECK_DEFAULT(kClickDurationRange, click_right.duration_ms);
	CHECK_DEFAULT(kUnitRange, click_sound.volume);

	CHECK_DEFAULT(kRadiusRange, spotlight.radius);
	CHECK_DEFAULT(kCornerRadiusRange, spotlight.corner_radius);
	CHECK_DEFAULT(kUnitRange, spotlight.dim);
	CHECK_DEFAULT(kBorderWidthRange, spotlight.border_width);
	CHECK_DEFAULT(kBgImageBlurRange, spotlight.blur_amount);
	CHECK_DEFAULT(kZoomAmountRange, spotlight.zoom_amount);
	CHECK_DEFAULT(kInOutMsRange, spotlight.in_ms);
	CHECK_DEFAULT(kInOutMsRange, spotlight.out_ms);

	CHECK_DEFAULT(kRadiusRange, magnifier.radius);
	CHECK_DEFAULT(kCornerRadiusRange, magnifier.corner_radius);
	CHECK_DEFAULT(kZoomAmountRange, magnifier.zoom_amount);
	CHECK_DEFAULT(kUnitRange, magnifier.center_x);
	CHECK_DEFAULT(kUnitRange, magnifier.center_y);
	CHECK_DEFAULT(kBorderWidthRange, magnifier.border_width);
	CHECK_DEFAULT(kUnitRange, magnifier.shadow_alpha);
	CHECK_DEFAULT(kShadowBlurRange, magnifier.shadow_blur);
	CHECK_DEFAULT(kShadowOffsetRange, magnifier.shadow_offset_x);
	CHECK_DEFAULT(kShadowOffsetRange, magnifier.shadow_offset_y);
	CHECK_DEFAULT(kInOutMsRange, magnifier.in_ms);
	CHECK_DEFAULT(kInOutMsRange, magnifier.out_ms);

	CHECK_DEFAULT(kKeysTtlRange, keys.ttl_ms);
	CHECK_DEFAULT(kKeysMaxRange, keys.max_keys);
	CHECK_DEFAULT(kUnitRange, keys.pos_x);
	CHECK_DEFAULT(kUnitRange, keys.pos_y);
	CHECK_DEFAULT(kKeysSizeRange, keys.size);

	CHECK_DEFAULT(kHighlightHoldRange, highlight.hold_ms);
	CHECK_DEFAULT(kHighlightFadeRange, highlight.fade_ms);
	CHECK_DEFAULT(kUnitRange, highlight.opacity);
	CHECK_DEFAULT(kHighlightThicknessRange, highlight.thickness);
	CHECK_DEFAULT(kHighlightHoldRange, highlight.zoom_delay_ms);
	CHECK_DEFAULT(kHighlightZoomMaxRange, highlight.zoom_max);
	CHECK_DEFAULT(kHighlightZoomInRange, highlight.zoom_in_ms);
	CHECK_DEFAULT(kHighlightHoldRange, highlight.zoom_hold_ms);
	CHECK_DEFAULT(kHighlightZoomOutRange, highlight.zoom_out_ms);

	CHECK_DEFAULT(kMonitorPositionRange, monitor_x);
	CHECK_DEFAULT(kMonitorPositionRange, monitor_y);

#undef CHECK_DEFAULT
}
