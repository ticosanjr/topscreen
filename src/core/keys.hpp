#pragma once

/**
 * Nomes das chaves de configuracao.
 *
 * Fonte unica de verdade, usada tanto pelo mapeamento obs_data (plugin) quanto
 * pelos arquivos de preset (nucleo).
 */
namespace topscreen::core::keys {

inline constexpr const char *kEffectsEnabled = "effects_enabled";

inline constexpr const char *kBgEnabled = "bg_enabled";
inline constexpr const char *kBgMode = "bg_mode";
inline constexpr const char *kBgColorA = "bg_color_a";
inline constexpr const char *kBgColorB = "bg_color_b";
inline constexpr const char *kBgImage = "bg_image";
inline constexpr const char *kBgImageFile = "bg_image_file";
inline constexpr const char *kBgPaddingX = "bg_padding_x";
inline constexpr const char *kBgPaddingY = "bg_padding_y";
inline constexpr const char *kBgCornerRadius = "bg_corner_radius";
inline constexpr const char *kBgShadow = "bg_shadow";
inline constexpr const char *kBgShadowAlpha = "bg_shadow_alpha";
inline constexpr const char *kBgShadowBlur = "bg_shadow_blur";
inline constexpr const char *kBgShadowOffset = "bg_shadow_offset";
inline constexpr const char *kBgKeepAspect = "bg_keep_aspect";

inline constexpr const char *kCursorEnabled = "cursor_enabled";
inline constexpr const char *kCursorStyle = "cursor_style";
inline constexpr const char *kCursorSize = "cursor_size";
inline constexpr const char *kCursorShadow = "cursor_shadow";
inline constexpr const char *kCursorSmooth = "cursor_smooth";
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
inline constexpr const char *kClickSoundFile = "click_sound_file";
inline constexpr const char *kClickSoundVolume = "click_sound_volume";
inline constexpr const char *kClickSoundLeft = "click_sound_left";
inline constexpr const char *kClickSoundRight = "click_sound_right";

inline constexpr const char *kMotionBlurEnabled = "motion_blur_enabled";
inline constexpr const char *kMotionBlurAmount = "motion_blur_amount";

inline constexpr const char *kSpotlightEnabled = "spotlight_enabled";
inline constexpr const char *kSpotlightRadius = "spotlight_radius";
inline constexpr const char *kSpotlightDim = "spotlight_dim";

inline constexpr const char *kKeysEnabled = "keys_enabled";
inline constexpr const char *kKeysModifiersOnly = "keys_modifiers_only";
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
inline constexpr const char *kHighlightHoldMs = "highlight_hold_ms";
inline constexpr const char *kHighlightFadeMs = "highlight_fade_ms";
inline constexpr const char *kHighlightColor = "highlight_color";

} // namespace topscreen::core::keys
