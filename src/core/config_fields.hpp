#pragma once

#include "keys.hpp"

/*
 * Single table of the persisted Config fields.
 *
 * X(kind, key, member) is expanded by the settings loader/store, by the preset
 * reader and by the defaults builder. Adding a Config field here updates every
 * path at once; fields with legacy handling (background type, click-sound
 * single file, spotlight outside mode) stay out of this list.
 */
#define TS_CONFIG_FIELDS(X) \
	X(BOOL, kEffectsEnabled, effects_enabled) \
	X(BOOL, kBgEnabled, bg.enabled) \
	X(COLOR, kBgColorA, bg.color_a) \
	X(COLOR, kBgColorB, bg.color_b) \
	X(DOUBLE, kBgGradientAngle, bg.gradient_angle) \
	X(DOUBLE, kBgGradientCenterX, bg.gradient_center_x) \
	X(DOUBLE, kBgGradientCenterY, bg.gradient_center_y) \
	X(STRING, kBgImage, bg.image) \
	X(STRING, kBgImageFile, bg.image_file) \
	X(DOUBLE, kBgImageBlur, bg.image_blur) \
	X(DOUBLE, kBgPaddingX, bg.padding_x) \
	X(DOUBLE, kBgPaddingY, bg.padding_y) \
	X(DOUBLE, kBgCornerRadius, bg.corner_radius) \
	X(BOOL, kBgShadow, bg.shadow) \
	X(DOUBLE, kBgShadowAlpha, bg.shadow_alpha) \
	X(DOUBLE, kBgShadowBlur, bg.shadow_blur) \
	X(DOUBLE, kBgShadowOffsetX, bg.shadow_offset_x) \
	X(DOUBLE, kBgShadowOffset, bg.shadow_offset) \
	X(BOOL, kBgKeepAspect, bg.keep_aspect) \
	X(INT, kFrameStyle, bg.frame_style) \
	X(COLOR, kFrameColor, bg.frame_color) \
	X(DOUBLE, kFrameWidth, bg.frame_width) \
	X(BOOL, kCursorEnabled, cursor.enabled) \
	X(INT, kCursorStyle, cursor.style) \
	X(INT, kCursorSecondaryStyle, cursor.secondary_style) \
	X(BOOL, kCursorLinkDetection, cursor.link_detection) \
	X(DOUBLE, kCursorSize, cursor.size) \
	X(DOUBLE, kCursorRotationOffset, cursor.rotation_offset_deg) \
	X(BOOL, kCursorShadow, cursor.shadow) \
	X(INT, kCursorFollowMode, cursor.follow_mode) \
	X(DOUBLE, kCursorGlideMs, cursor.glide_ms) \
	X(DOUBLE, kCursorSmoothMs, cursor.smooth_ms) \
	X(BOOL, kCursorHideIdle, cursor.hide_idle) \
	X(DOUBLE, kCursorIdleDelayMs, cursor.idle_delay_ms) \
	X(INT, kZoomMode, zoom.mode) \
	X(DOUBLE, kZoomAmount, zoom.amount) \
	X(DOUBLE, kZoomDurationMs, zoom.duration_ms) \
	X(DOUBLE, kZoomIdleTimeoutMs, zoom.idle_timeout_ms) \
	X(DOUBLE, kZoomSmartHoldMs, zoom.smart_hold_ms) \
	X(BOOL, kZoomActiveWindow, zoom.active_window) \
	X(BOOL, kZoomMotionBlur, zoom.motion_blur) \
	X(DOUBLE, kZoomMotionBlurAmount, zoom.motion_blur_amount) \
	X(DOUBLE, kZoomPanAmount, zoom.pan_amount) \
	X(DOUBLE, kZoomCenterAmount, zoom.center_amount) \
	X(INT, kClickLeftEffect, click_left.effect) \
	X(DOUBLE, kClickLeftSize, click_left.size) \
	X(COLOR, kClickLeftColor, click_left.color) \
	X(DOUBLE, kClickLeftDurationMs, click_left.duration_ms) \
	X(INT, kClickRightEffect, click_right.effect) \
	X(DOUBLE, kClickRightSize, click_right.size) \
	X(COLOR, kClickRightColor, click_right.color) \
	X(DOUBLE, kClickRightDurationMs, click_right.duration_ms) \
	X(BOOL, kClickSoundEnabled, click_sound.enabled) \
	X(STRING, kClickSoundFileLeft, click_sound.file_left) \
	X(STRING, kClickSoundFileRight, click_sound.file_right) \
	X(DOUBLE, kClickSoundVolume, click_sound.volume) \
	X(BOOL, kClickSoundLeft, click_sound.left) \
	X(BOOL, kClickSoundRight, click_sound.right) \
	X(BOOL, kSpotlightEnabled, spotlight.enabled) \
	X(DOUBLE, kSpotlightRadius, spotlight.radius) \
	X(DOUBLE, kSpotlightDim, spotlight.dim) \
	X(INT, kSpotlightShape, spotlight.shape) \
	X(DOUBLE, kSpotlightCornerRadius, spotlight.corner_radius) \
	X(BOOL, kSpotlightBorderEnabled, spotlight.border_enabled) \
	X(COLOR, kSpotlightBorderColor, spotlight.border_color) \
	X(DOUBLE, kSpotlightBorderWidth, spotlight.border_width) \
	X(BOOL, kSpotlightBlurEnabled, spotlight.blur_enabled) \
	X(INT, kSpotlightOutsideMode, spotlight.outside_mode) \
	X(DOUBLE, kSpotlightBlurAmount, spotlight.blur_amount) \
	X(DOUBLE, kSpotlightZoomAmount, spotlight.zoom_amount) \
	X(DOUBLE, kSpotlightInMs, spotlight.in_ms) \
	X(DOUBLE, kSpotlightOutMs, spotlight.out_ms) \
	X(BOOL, kSpotlightShowCursor, spotlight.show_cursor) \
	X(BOOL, kMagnifierEnabled, magnifier.enabled) \
	X(INT, kMagnifierShape, magnifier.shape) \
	X(DOUBLE, kMagnifierRadius, magnifier.radius) \
	X(DOUBLE, kMagnifierCornerRadius, magnifier.corner_radius) \
	X(DOUBLE, kMagnifierZoom, magnifier.zoom_amount) \
	X(BOOL, kMagnifierBorderEnabled, magnifier.border_enabled) \
	X(COLOR, kMagnifierBorderColor, magnifier.border_color) \
	X(DOUBLE, kMagnifierBorderWidth, magnifier.border_width) \
	X(BOOL, kMagnifierShadowEnabled, magnifier.shadow_enabled) \
	X(COLOR, kMagnifierShadowColor, magnifier.shadow_color) \
	X(DOUBLE, kMagnifierShadowAlpha, magnifier.shadow_alpha) \
	X(DOUBLE, kMagnifierShadowBlur, magnifier.shadow_blur) \
	X(DOUBLE, kMagnifierShadowOffsetX, magnifier.shadow_offset_x) \
	X(DOUBLE, kMagnifierShadowOffsetY, magnifier.shadow_offset_y) \
	X(BOOL, kMagnifierFollowCursor, magnifier.follow_cursor) \
	X(DOUBLE, kMagnifierCenterX, magnifier.center_x) \
	X(DOUBLE, kMagnifierCenterY, magnifier.center_y) \
	X(DOUBLE, kMagnifierInMs, magnifier.in_ms) \
	X(DOUBLE, kMagnifierOutMs, magnifier.out_ms) \
	X(BOOL, kMagnifierShowCursor, magnifier.show_cursor) \
	X(BOOL, kKeysEnabled, keys.enabled) \
	X(BOOL, kKeysModifiersOnly, keys.modifiers_only) \
	X(BOOL, kKeysShortcutsOnly, keys.shortcuts_only) \
	X(BOOL, kKeysShowSpecial, keys.show_special) \
	X(BOOL, kKeysShowShortcuts, keys.show_shortcuts) \
	X(DOUBLE, kKeysTtlMs, keys.ttl_ms) \
	X(INT, kKeysMax, keys.max_keys) \
	X(DOUBLE, kKeysPosX, keys.pos_x) \
	X(DOUBLE, kKeysPosY, keys.pos_y) \
	X(DOUBLE, kKeysSize, keys.size) \
	X(STRING, kKeysFont, keys.font) \
	X(COLOR, kKeysColor, keys.color) \
	X(BOOL, kKeysShadow, keys.shadow) \
	X(BOOL, kKeysBand, keys.band) \
	X(COLOR, kKeysBandColor, keys.band_color) \
	X(BOOL, kHighlightEnabled, highlight.enabled) \
	X(BOOL, kHighlightHotkeyEnabled, highlight.hotkey_enabled) \
	X(DOUBLE, kHighlightHoldMs, highlight.hold_ms) \
	X(DOUBLE, kHighlightFadeMs, highlight.fade_ms) \
	X(COLOR, kHighlightColor, highlight.color) \
	X(DOUBLE, kHighlightOpacity, highlight.opacity) \
	X(INT, kHighlightMode, highlight.mode) \
	X(BOOL, kHighlightLockUntilClick, highlight.lock_until_click) \
	X(DOUBLE, kHighlightReadingDim, highlight.reading_dim) \
	X(DOUBLE, kHighlightThickness, highlight.thickness) \
	X(DOUBLE, kHighlightZoomDelayMs, highlight.zoom_delay_ms) \
	X(DOUBLE, kHighlightZoomMax, highlight.zoom_max) \
	X(DOUBLE, kHighlightZoomInMs, highlight.zoom_in_ms) \
	X(DOUBLE, kHighlightZoomHoldMs, highlight.zoom_hold_ms) \
	X(DOUBLE, kHighlightZoomOutMs, highlight.zoom_out_ms) \
	X(BOOL, kMonitorManual, monitor_manual) \
	X(INT, kMonitorX, monitor_x) \
	X(INT, kMonitorY, monitor_y)
