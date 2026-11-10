#include "config.hpp"

#include <algorithm>
#include <cmath>

#include "config_limits.hpp"
#include "cursor_catalog.hpp"
#include "highlighter.hpp"
#include "types.hpp"

namespace topscreen::core {

namespace {

double bounded(double value, const limits::Range &range, double fallback)
{
	if (!std::isfinite(value))
		value = fallback;
	return std::clamp(value, range.min, range.max);
}

int bounded_int(int value, const limits::IntRange &range)
{
	return std::clamp(value, range.min, range.max);
}

int bounded_int(int value, int lo, int hi)
{
	return std::clamp(value, lo, hi);
}

/* Keeps -45° and 315° equivalent while still rejecting non-finite input. */
double wrapped_angle(double value, double fallback)
{
	if (!std::isfinite(value))
		value = fallback;
	value = std::fmod(value, limits::kBgGradientAngleRange.max);
	if (value < 0.0)
		value += limits::kBgGradientAngleRange.max;
	return value;
}

void sanitize_click(ClickConfig *c, const ClickConfig &def)
{
	c->effect = bounded_int(c->effect, static_cast<int>(ClickEffect::Ripple),
				static_cast<int>(ClickEffect::WaterRippleClear));
	c->size = bounded(c->size, limits::kClickSizeRange, def.size);
	c->duration_ms = bounded(c->duration_ms, limits::kClickDurationRange, def.duration_ms);
}

} // namespace

void sanitize_config(Config *cfg)
{
	if (!cfg)
		return;

	const Config def;

	/* Background */
	cfg->bg.mode = bounded_int(cfg->bg.mode, static_cast<int>(BgMode::Solid),
				   static_cast<int>(BgMode::BlurredScreen));
	cfg->bg.gradient_angle = wrapped_angle(cfg->bg.gradient_angle, def.bg.gradient_angle);
	cfg->bg.gradient_center_x =
		bounded(cfg->bg.gradient_center_x, limits::kUnitRange, def.bg.gradient_center_x);
	cfg->bg.gradient_center_y =
		bounded(cfg->bg.gradient_center_y, limits::kUnitRange, def.bg.gradient_center_y);
	cfg->bg.image_blur = bounded(cfg->bg.image_blur, limits::kBgImageBlurRange, def.bg.image_blur);
	cfg->bg.padding_x = bounded(cfg->bg.padding_x, limits::kBgPaddingRange, def.bg.padding_x);
	cfg->bg.padding_y = bounded(cfg->bg.padding_y, limits::kBgPaddingRange, def.bg.padding_y);
	cfg->bg.corner_radius =
		bounded(cfg->bg.corner_radius, limits::kCornerRadiusRange, def.bg.corner_radius);
	cfg->bg.shadow_alpha = bounded(cfg->bg.shadow_alpha, limits::kUnitRange, def.bg.shadow_alpha);
	cfg->bg.shadow_blur = bounded(cfg->bg.shadow_blur, limits::kShadowBlurRange, def.bg.shadow_blur);
	cfg->bg.shadow_offset_x =
		bounded(cfg->bg.shadow_offset_x, limits::kShadowOffsetRange, def.bg.shadow_offset_x);
	cfg->bg.shadow_offset =
		bounded(cfg->bg.shadow_offset, limits::kShadowOffsetRange, def.bg.shadow_offset);

	/* Cursor */
	cfg->cursor.style = cursor_style_resolve(cfg->cursor.style);
	if (cfg->cursor.secondary_style < -1)
		cfg->cursor.secondary_style = -1;
	else if (cfg->cursor.secondary_style >= 0)
		cfg->cursor.secondary_style = cursor_style_resolve(cfg->cursor.secondary_style);
	/* Spring and sampled glide were removed; legacy values 1/2 map to A→B.
	 * Exact and Smooth keep their own mode; invalid values map to A→B. */
	if (cfg->cursor.follow_mode < static_cast<int>(CursorFollowMode::Exact) ||
	    cfg->cursor.follow_mode > static_cast<int>(CursorFollowMode::Smooth))
		cfg->cursor.follow_mode = static_cast<int>(CursorFollowMode::GlideAB);
	else if (cfg->cursor.follow_mode != static_cast<int>(CursorFollowMode::Exact) &&
		 cfg->cursor.follow_mode != static_cast<int>(CursorFollowMode::Smooth))
		cfg->cursor.follow_mode = static_cast<int>(CursorFollowMode::GlideAB);
	cfg->cursor.size = bounded(cfg->cursor.size, limits::kCursorSizeRange, def.cursor.size);
	cfg->cursor.rotation_offset_deg =
		bounded(cfg->cursor.rotation_offset_deg, limits::kCursorRotationRange, 0.0);
	cfg->cursor.glide_ms = bounded(cfg->cursor.glide_ms, limits::kCursorGlideRange, def.cursor.glide_ms);
	cfg->cursor.smooth_ms = bounded(cfg->cursor.smooth_ms, limits::kCursorSmoothRange,
					 def.cursor.smooth_ms);
	cfg->cursor.idle_delay_ms =
		bounded(cfg->cursor.idle_delay_ms, limits::kCursorIdleDelayRange, def.cursor.idle_delay_ms);

	/* Zoom */
	/* Retired "Manual" (2) is a plain Off; hotkeys still adjust the manual
	 * zoom independently of the mode. */
	if (cfg->zoom.mode == 2)
		cfg->zoom.mode = static_cast<int>(ZoomMode::Off);
	cfg->zoom.mode = bounded_int(cfg->zoom.mode, static_cast<int>(ZoomMode::Off),
				     static_cast<int>(ZoomMode::Smart));
	cfg->zoom.amount = bounded(cfg->zoom.amount, limits::kZoomAmountRange, def.zoom.amount);
	cfg->zoom.duration_ms = bounded(cfg->zoom.duration_ms, limits::kZoomDurationRange, def.zoom.duration_ms);
	cfg->zoom.idle_timeout_ms =
		bounded(cfg->zoom.idle_timeout_ms, limits::kZoomIdleTimeoutRange, def.zoom.idle_timeout_ms);
	cfg->zoom.smart_hold_ms = bounded(cfg->zoom.smart_hold_ms, limits::kZoomSmartHoldRange,
					   def.zoom.smart_hold_ms);
	cfg->zoom.motion_blur_amount = bounded(cfg->zoom.motion_blur_amount, limits::kUnitRange,
					       def.zoom.motion_blur_amount);
	cfg->zoom.pan_amount = bounded(cfg->zoom.pan_amount, limits::kUnitRange, def.zoom.pan_amount);
	cfg->zoom.center_amount = bounded(cfg->zoom.center_amount, limits::kUnitRange, def.zoom.center_amount);

	/* Clicks */
	sanitize_click(&cfg->click_left, def.click_left);
	sanitize_click(&cfg->click_right, def.click_right);
	cfg->click_sound.volume =
		bounded(cfg->click_sound.volume, limits::kUnitRange, def.click_sound.volume);

	/* Spotlight */
	cfg->spotlight.shape = bounded_int(cfg->spotlight.shape,
					   static_cast<int>(SpotlightShape::Circle),
					   static_cast<int>(SpotlightShape::Square));
	cfg->spotlight.radius = bounded(cfg->spotlight.radius, limits::kRadiusRange, def.spotlight.radius);
	cfg->spotlight.corner_radius =
		bounded(cfg->spotlight.corner_radius, limits::kCornerRadiusRange, def.spotlight.corner_radius);
	cfg->spotlight.dim = bounded(cfg->spotlight.dim, limits::kUnitRange, def.spotlight.dim);
	cfg->spotlight.border_width =
		bounded(cfg->spotlight.border_width, limits::kBorderWidthRange, def.spotlight.border_width);
	cfg->spotlight.blur_amount =
		bounded(cfg->spotlight.blur_amount, limits::kBgImageBlurRange, def.spotlight.blur_amount);
	cfg->spotlight.outside_mode = bounded_int(cfg->spotlight.outside_mode, 0, 1);
	/* Keep the deprecated flag in sync for older builds that still read it.
	 * Do NOT use it to override outside_mode: that made the Dim choice
	 * impossible whenever an old profile had blur enabled. */
	cfg->spotlight.blur_enabled = cfg->spotlight.outside_mode == 1;
	cfg->spotlight.zoom_amount =
		bounded(cfg->spotlight.zoom_amount, limits::kZoomAmountRange, def.spotlight.zoom_amount);
	cfg->spotlight.in_ms = bounded(cfg->spotlight.in_ms, limits::kInOutMsRange, def.spotlight.in_ms);
	cfg->spotlight.out_ms = bounded(cfg->spotlight.out_ms, limits::kInOutMsRange, def.spotlight.out_ms);

	/* Magnifier */
	cfg->magnifier.shape = bounded_int(cfg->magnifier.shape,
					   static_cast<int>(SpotlightShape::Circle),
					   static_cast<int>(SpotlightShape::Square));
	cfg->magnifier.radius =
		bounded(cfg->magnifier.radius, limits::kRadiusRange, def.magnifier.radius);
	cfg->magnifier.corner_radius =
		bounded(cfg->magnifier.corner_radius, limits::kCornerRadiusRange, def.magnifier.corner_radius);
	cfg->magnifier.zoom_amount =
		bounded(cfg->magnifier.zoom_amount, limits::kZoomAmountRange, def.magnifier.zoom_amount);
	cfg->magnifier.center_x =
		bounded(cfg->magnifier.center_x, limits::kUnitRange, def.magnifier.center_x);
	cfg->magnifier.center_y =
		bounded(cfg->magnifier.center_y, limits::kUnitRange, def.magnifier.center_y);
	cfg->magnifier.border_width =
		bounded(cfg->magnifier.border_width, limits::kBorderWidthRange, def.magnifier.border_width);
	cfg->magnifier.shadow_alpha =
		bounded(cfg->magnifier.shadow_alpha, limits::kUnitRange, def.magnifier.shadow_alpha);
	cfg->magnifier.shadow_blur =
		bounded(cfg->magnifier.shadow_blur, limits::kShadowBlurRange, def.magnifier.shadow_blur);
	cfg->magnifier.shadow_offset_x =
		bounded(cfg->magnifier.shadow_offset_x, limits::kShadowOffsetRange, def.magnifier.shadow_offset_x);
	cfg->magnifier.shadow_offset_y =
		bounded(cfg->magnifier.shadow_offset_y, limits::kShadowOffsetRange, def.magnifier.shadow_offset_y);
	cfg->magnifier.in_ms =
		bounded(cfg->magnifier.in_ms, limits::kInOutMsRange, def.magnifier.in_ms);
	cfg->magnifier.out_ms =
		bounded(cfg->magnifier.out_ms, limits::kInOutMsRange, def.magnifier.out_ms);

	/* The two full-screen focus effects are mutually exclusive by design. */
	if (cfg->magnifier.enabled)
		cfg->spotlight.enabled = false;

	/* Key overlay */
	cfg->keys.ttl_ms = bounded(cfg->keys.ttl_ms, limits::kKeysTtlRange, def.keys.ttl_ms);
	cfg->keys.max_keys = bounded_int(cfg->keys.max_keys, limits::kKeysMaxRange);
	cfg->keys.pos_x = bounded(cfg->keys.pos_x, limits::kUnitRange, def.keys.pos_x);
	cfg->keys.pos_y = bounded(cfg->keys.pos_y, limits::kUnitRange, def.keys.pos_y);
	cfg->keys.size = bounded(cfg->keys.size, limits::kKeysSizeRange, def.keys.size);

	/* Highlighter */
	/* PressZoom (legacy value 5) was removed: migrate to plain Zoom. */
	if (cfg->highlight.mode == 5)
		cfg->highlight.mode = static_cast<int>(HighlightMode::Zoom);
	cfg->highlight.mode = bounded_int(cfg->highlight.mode,
					  static_cast<int>(HighlightMode::Box),
					  static_cast<int>(HighlightMode::LineZoom));
	cfg->highlight.hold_ms = bounded(cfg->highlight.hold_ms, limits::kHighlightHoldRange, def.highlight.hold_ms);
	cfg->highlight.fade_ms = bounded(cfg->highlight.fade_ms, limits::kHighlightFadeRange, def.highlight.fade_ms);
	cfg->highlight.opacity = bounded(cfg->highlight.opacity, limits::kUnitRange, def.highlight.opacity);
	cfg->highlight.thickness = bounded(cfg->highlight.thickness, limits::kHighlightThicknessRange,
					   def.highlight.thickness);
	cfg->highlight.zoom_delay_ms =
		bounded(cfg->highlight.zoom_delay_ms, limits::kHighlightHoldRange,
			def.highlight.zoom_delay_ms);
	cfg->highlight.zoom_max = bounded(cfg->highlight.zoom_max, limits::kHighlightZoomMaxRange, def.highlight.zoom_max);
	cfg->highlight.zoom_in_ms = bounded(cfg->highlight.zoom_in_ms, limits::kHighlightZoomInRange,
					    def.highlight.zoom_in_ms);
	cfg->highlight.zoom_hold_ms =
		bounded(cfg->highlight.zoom_hold_ms, limits::kHighlightHoldRange,
			def.highlight.zoom_hold_ms);
	cfg->highlight.zoom_out_ms = bounded(cfg->highlight.zoom_out_ms,
					     limits::kHighlightZoomOutRange, def.highlight.zoom_out_ms);
}

} // namespace topscreen::core
