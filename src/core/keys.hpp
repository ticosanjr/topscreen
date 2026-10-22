#pragma once

/**
 * Configuration key names.
 *
 * Single source of truth, used both by the obs_data mapping (plugin) and
 * by the preset files (core).
 */
namespace topscreen::core::keys {

inline constexpr const char *kEffectsEnabled = "effects_enabled";

inline constexpr const char *kBgEnabled = "bg_enabled";
inline constexpr const char *kBgMode = "bg_mode"; // legacy storage
inline constexpr const char *kBgType = "bg_type"; // 0 solid, 1 gradient, 2 wallpaper
inline constexpr const char *kBgColorA = "bg_color_a";
inline constexpr const char *kBgColorB = "bg_color_b";
inline constexpr const char *kBgGradientAngle = "bg_gradient_angle";
inline constexpr const char *kBgGradientCenterX = "bg_gradient_center_x";
inline constexpr const char *kBgGradientCenterY = "bg_gradient_center_y";
inline constexpr const char *kBgImage = "bg_image";
inline constexpr const char *kBgImageFile = "bg_image_file";
inline constexpr const char *kBgImageBlur = "bg_image_blur";
inline constexpr const char *kBgImageBlurType = "bg_image_blur_type";
inline constexpr const char *kBgPaddingX = "bg_padding_x";
inline constexpr const char *kBgPaddingY = "bg_padding_y";
inline constexpr const char *kBgCornerRadius = "bg_corner_radius";
inline constexpr const char *kBgShadow = "bg_shadow";
inline constexpr const char *kBgShadowAlpha = "bg_shadow_alpha";
inline constexpr const char *kBgShadowBlur = "bg_shadow_blur";
inline constexpr const char *kBgShadowOffsetX = "bg_shadow_offset_x";
inline constexpr const char *kBgShadowOffset = "bg_shadow_offset";
inline constexpr const char *kBgKeepAspect = "bg_keep_aspect";

inline constexpr const char *kCursorEnabled = "cursor_enabled";
inline constexpr const char *kCursorStyle = "cursor_style";
inline constexpr const char *kCursorSecondaryStyle = "cursor_secondary_style";
inline constexpr const char *kCursorLinkDetection = "cursor_link_detection";
inline constexpr const char *kCursorSize = "cursor_size";
inline constexpr const char *kCursorShadow = "cursor_shadow";
inline constexpr const char *kCursorFollowMode = "cursor_follow_mode";
inline constexpr const char *kCursorSmooth = "cursor_smooth";
inline constexpr const char *kCursorGlideMs = "cursor_glide_ms";
inline constexpr const char *kCursorGlideSampleMs = "cursor_glide_sample_ms";
inline constexpr const char *kCursorSway = "cursor_sway";
inline constexpr const char *kCursorHideIdle = "cursor_hide_idle";
inline constexpr const char *kCursorIdleDelayMs = "cursor_idle_delay_ms";
inline constexpr const char *kCursorLoop = "cursor_loop";
inline constexpr const char *kCursorLoopMs = "cursor_loop_ms";

inline constexpr const char *kZoomMode = "zoom_mode";
inline constexpr const char *kZoomAmount = "zoom_amount";
inline constexpr const char *kZoomDurationMs = "zoom_duration_ms";
inline constexpr const char *kZoomIdleTimeoutMs = "zoom_idle_timeout_ms";
inline constexpr const char *kZoomPanAmount = "zoom_pan_amount";
inline constexpr const char *kZoomCenterAmount = "zoom_center_amount";

inline constexpr const char *kClickLeftEffect = "click_left_effect";
inline constexpr const char *kClickLeftSize = "click_left_size";
inline constexpr const char *kClickLeftColor = "click_left_color";
inline constexpr const char *kClickLeftDurationMs = "click_left_duration_ms";
inline constexpr const char *kClickLeftEasing = "click_left_easing";

inline constexpr const char *kClickRightEffect = "click_right_effect";
inline constexpr const char *kClickRightSize = "click_right_size";
inline constexpr const char *kClickRightColor = "click_right_color";
inline constexpr const char *kClickRightDurationMs = "click_right_duration_ms";
inline constexpr const char *kClickRightEasing = "click_right_easing";

inline constexpr const char *kClickSoundEnabled = "click_sound_enabled";
inline constexpr const char *kClickSoundFile = "click_sound_file"; // legacy (migrates to left/right)
inline constexpr const char *kClickSoundFileLeft = "click_sound_file_left";
inline constexpr const char *kClickSoundFileRight = "click_sound_file_right";
inline constexpr const char *kClickSoundVolume = "click_sound_volume";
inline constexpr const char *kClickSoundLeft = "click_sound_left";
inline constexpr const char *kClickSoundRight = "click_sound_right";

inline constexpr const char *kMotionBlurEnabled = "motion_blur_enabled";
inline constexpr const char *kMotionBlurAmount = "motion_blur_amount";

inline constexpr const char *kSpotlightEnabled = "spotlight_enabled";
inline constexpr const char *kSpotlightRadius = "spotlight_radius";
inline constexpr const char *kSpotlightDim = "spotlight_dim";
inline constexpr const char *kSpotlightShape = "spotlight_shape";
inline constexpr const char *kSpotlightCornerRadius = "spotlight_corner_radius";
inline constexpr const char *kSpotlightBorderEnabled = "spotlight_border_enabled";
inline constexpr const char *kSpotlightBorderColor = "spotlight_border_color";
inline constexpr const char *kSpotlightBorderWidth = "spotlight_border_width";
inline constexpr const char *kSpotlightShadowEnabled = "spotlight_shadow_enabled";
inline constexpr const char *kSpotlightShadowColor = "spotlight_shadow_color";
inline constexpr const char *kSpotlightShadowAlpha = "spotlight_shadow_alpha";
inline constexpr const char *kSpotlightShadowBlur = "spotlight_shadow_blur";
inline constexpr const char *kSpotlightShadowOffsetX = "spotlight_shadow_offset_x";
inline constexpr const char *kSpotlightShadowOffsetY = "spotlight_shadow_offset_y";
inline constexpr const char *kSpotlightBlurEnabled = "spotlight_blur_enabled";
inline constexpr const char *kSpotlightBlurAmount = "spotlight_blur_amount";
inline constexpr const char *kSpotlightZoomAmount = "spotlight_zoom_amount";
inline constexpr const char *kSpotlightInMs = "spotlight_in_ms";
inline constexpr const char *kSpotlightOutMs = "spotlight_out_ms";
inline constexpr const char *kSpotlightDropPx = "spotlight_drop_px";

inline constexpr const char *kMagnifierEnabled = "magnifier_enabled";
inline constexpr const char *kMagnifierShape = "magnifier_shape";
inline constexpr const char *kMagnifierRadius = "magnifier_radius";
inline constexpr const char *kMagnifierCornerRadius = "magnifier_corner_radius";
inline constexpr const char *kMagnifierZoom = "magnifier_zoom";
inline constexpr const char *kMagnifierBorderEnabled = "magnifier_border_enabled";
inline constexpr const char *kMagnifierBorderColor = "magnifier_border_color";
inline constexpr const char *kMagnifierBorderWidth = "magnifier_border_width";
inline constexpr const char *kMagnifierShadowEnabled = "magnifier_shadow_enabled";
inline constexpr const char *kMagnifierShadowColor = "magnifier_shadow_color";
inline constexpr const char *kMagnifierShadowAlpha = "magnifier_shadow_alpha";
inline constexpr const char *kMagnifierShadowBlur = "magnifier_shadow_blur";
inline constexpr const char *kMagnifierShadowOffsetX = "magnifier_shadow_offset_x";
inline constexpr const char *kMagnifierShadowOffsetY = "magnifier_shadow_offset_y";
inline constexpr const char *kMagnifierFollowCursor = "magnifier_follow_cursor";
inline constexpr const char *kMagnifierCenterX = "magnifier_center_x";
inline constexpr const char *kMagnifierCenterY = "magnifier_center_y";
inline constexpr const char *kMagnifierInMs = "magnifier_in_ms";
inline constexpr const char *kMagnifierOutMs = "magnifier_out_ms";
inline constexpr const char *kMagnifierDropPx = "magnifier_drop_px";

inline constexpr const char *kKeysEnabled = "keys_enabled";
inline constexpr const char *kKeysModifiersOnly = "keys_modifiers_only";
inline constexpr const char *kKeysShowSpecial = "keys_show_special";
inline constexpr const char *kKeysTtlMs = "keys_ttl_ms";
inline constexpr const char *kKeysMax = "keys_max";
inline constexpr const char *kKeysPosX = "keys_pos_x";
inline constexpr const char *kKeysPosY = "keys_pos_y";
inline constexpr const char *kKeysSize = "keys_size";
inline constexpr const char *kKeysFont = "keys_font";
inline constexpr const char *kKeysColor = "keys_color";
inline constexpr const char *kKeysShadow = "keys_shadow";
inline constexpr const char *kKeysBand = "keys_band";
inline constexpr const char *kKeysBandColor = "keys_band_color";

inline constexpr const char *kHighlightEnabled = "highlight_enabled";
inline constexpr const char *kHighlightHotkeyEnabled = "highlight_hotkey_enabled";
inline constexpr const char *kHighlightHoldMs = "highlight_hold_ms";
inline constexpr const char *kHighlightFadeMs = "highlight_fade_ms";
inline constexpr const char *kHighlightColor = "highlight_color";
inline constexpr const char *kHighlightOpacity = "highlight_opacity";
inline constexpr const char *kHighlightMode = "highlight_mode";
inline constexpr const char *kHighlightThickness = "highlight_thickness";
inline constexpr const char *kHighlightZoomMax = "highlight_zoom_max";
inline constexpr const char *kHighlightZoomDir = "highlight_zoom_dir";
inline constexpr const char *kHighlightZoomDelayMs = "highlight_zoom_delay_ms";
inline constexpr const char *kHighlightZoomInMs = "highlight_zoom_in_ms";
inline constexpr const char *kHighlightZoomHoldMs = "highlight_zoom_hold_ms";
inline constexpr const char *kHighlightZoomOutMs = "highlight_zoom_out_ms";
inline constexpr const char *kHighlightPressDelayMs = "highlight_press_delay_ms";

/* Monitor origin (multi-monitor input mapping). */
inline constexpr const char *kMonitorManual = "monitor_manual";
inline constexpr const char *kMonitorX = "monitor_x";
inline constexpr const char *kMonitorY = "monitor_y";

/* Profiles (fixed slots). Each slot stores the entire Config in a nested
 * obs_data. Labels run A..J (see core::profile_label). */
inline constexpr int kProfileCount = 10;
inline constexpr const char *kProfileCurrent = "profile_current";
inline constexpr const char *kProfileSlots[kProfileCount] = {
	"profile_slot_0", "profile_slot_1", "profile_slot_2", "profile_slot_3", "profile_slot_4",
	"profile_slot_5", "profile_slot_6", "profile_slot_7", "profile_slot_8", "profile_slot_9",
};

} // namespace topscreen::core::keys
