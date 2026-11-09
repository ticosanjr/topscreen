#include "ts-settings.hpp"

#include "core/cursor_catalog.hpp"
#include "core/keys.hpp"
#include "ts-wallpapers.hpp"

#include <algorithm>

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen {

#define TS_SIMPLE_FIELDS(X) \
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
	X(BOOL, kCursorEnabled, cursor.enabled) \
	X(INT, kCursorStyle, cursor.style) \
	X(INT, kCursorSecondaryStyle, cursor.secondary_style) \
	X(BOOL, kCursorLinkDetection, cursor.link_detection) \
	X(DOUBLE, kCursorSize, cursor.size) \
	X(DOUBLE, kCursorRotationOffset, cursor.rotation_offset_deg) \
	X(BOOL, kCursorShadow, cursor.shadow) \
	X(INT, kCursorFollowMode, cursor.follow_mode) \
	X(DOUBLE, kCursorGlideMs, cursor.glide_ms) \
	X(BOOL, kCursorHideIdle, cursor.hide_idle) \
	X(DOUBLE, kCursorIdleDelayMs, cursor.idle_delay_ms) \
	X(INT, kZoomMode, zoom.mode) \
	X(DOUBLE, kZoomAmount, zoom.amount) \
	X(DOUBLE, kZoomDurationMs, zoom.duration_ms) \
	X(DOUBLE, kZoomIdleTimeoutMs, zoom.idle_timeout_ms) \
	X(DOUBLE, kZoomPanAmount, zoom.pan_amount) \
	X(DOUBLE, kZoomCenterAmount, zoom.center_amount) \
	X(INT, kClickLeftEffect, click_left.effect) \
	X(DOUBLE, kClickLeftSize, click_left.size) \
	X(COLOR, kClickLeftColor, click_left.color) \
	X(DOUBLE, kClickLeftDurationMs, click_left.duration_ms) \
	X(INT, kClickLeftEasing, click_left.easing) \
	X(INT, kClickRightEffect, click_right.effect) \
	X(DOUBLE, kClickRightSize, click_right.size) \
	X(COLOR, kClickRightColor, click_right.color) \
	X(DOUBLE, kClickRightDurationMs, click_right.duration_ms) \
	X(INT, kClickRightEasing, click_right.easing) \
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
	X(DOUBLE, kHighlightThickness, highlight.thickness) \
	X(DOUBLE, kHighlightZoomDelayMs, highlight.zoom_delay_ms) \
	X(DOUBLE, kHighlightZoomMax, highlight.zoom_max) \
	X(DOUBLE, kHighlightZoomInMs, highlight.zoom_in_ms) \
	X(DOUBLE, kHighlightZoomHoldMs, highlight.zoom_hold_ms) \
	X(DOUBLE, kHighlightZoomOutMs, highlight.zoom_out_ms) \
	X(BOOL, kMonitorManual, monitor_manual) \
	X(INT, kMonitorX, monitor_x) \
	X(INT, kMonitorY, monitor_y)

#define TS_DEF_BOOL(k, m) obs_data_set_default_bool(s, k, def.m)
#define TS_DEF_INT(k, m) obs_data_set_default_int(s, k, def.m)
#define TS_DEF_DOUBLE(k, m) obs_data_set_default_double(s, k, def.m)
#define TS_DEF_COLOR(k, m) obs_data_set_default_int(s, k, static_cast<long long>(def.m))
#define TS_DEF_STRING(k, m) obs_data_set_default_string(s, k, def.m.c_str())

void config_defaults(obs_data_t *s)
{
	const Config def;
#define X(kind, key, member) TS_DEF_##kind(key, member);
	TS_SIMPLE_FIELDS(X)
#undef X
	obs_data_set_default_int(s, kBgType, def.bg.mode);
	obs_data_set_default_int(s, kBgMode, legacy_bg_mode_value(def.bg.mode));
	obs_data_set_default_int(s, kProfileCurrent, 0);
}

namespace {

/*
 * Read with fallback to the code defaults.
 *
 * Sources created by old versions do not have the new keys; without this they
 * would become 0/false (padding 0, invisible text, etc.).
 */
bool has_key(obs_data_t *s, const char *k)
{
	return obs_data_has_user_value(s, k) || obs_data_has_default_value(s, k);
}
bool read_bool(obs_data_t *s, const char *k, bool d)
{
	return has_key(s, k) ? obs_data_get_bool(s, k) : d;
}
long long read_int(obs_data_t *s, const char *k, long long d)
{
	return has_key(s, k) ? obs_data_get_int(s, k) : d;
}
double read_double(obs_data_t *s, const char *k, double d)
{
	return has_key(s, k) ? obs_data_get_double(s, k) : d;
}
std::string read_string(obs_data_t *s, const char *k, const std::string &d)
{
	if (!has_key(s, k))
		return d;
	const char *v = obs_data_get_string(s, k);
	return v ? v : d;
}

} // namespace

#define TS_LOAD_BOOL(k, m) c->m = read_bool(s, k, d.m)
#define TS_LOAD_INT(k, m) c->m = static_cast<int>(read_int(s, k, d.m))
#define TS_LOAD_DOUBLE(k, m) c->m = read_double(s, k, d.m)
#define TS_LOAD_COLOR(k, m) c->m = static_cast<uint32_t>(read_int(s, k, d.m))
#define TS_LOAD_STRING(k, m) c->m = read_string(s, k, d.m)

#define TS_STORE_BOOL(k, m) obs_data_set_bool(s, k, c.m)
#define TS_STORE_INT(k, m) obs_data_set_int(s, k, c.m)
#define TS_STORE_DOUBLE(k, m) obs_data_set_double(s, k, c.m)
#define TS_STORE_COLOR(k, m) obs_data_set_int(s, k, static_cast<long long>(c.m))
#define TS_STORE_STRING(k, m) obs_data_set_string(s, k, c.m.c_str())

void config_load(obs_data_t *s, Config *c, const Config *fallback)
{
	const Config d = fallback ? *fallback : Config();

#define X(kind, key, member) TS_LOAD_##kind(key, member);
	TS_SIMPLE_FIELDS(X)
#undef X

	const long long legacy_mode = read_int(s, kBgMode, legacy_bg_mode_value(d.bg.mode));
	const bool has_type = obs_data_has_user_value(s, kBgType);
	int mode = static_cast<int>(d.bg.mode);
	if (has_type) {
		mode = static_cast<int>(read_int(s, kBgType, d.bg.mode));
	} else {
		const bool has_wallpaper = !c->bg.image.empty() || !c->bg.image_file.empty();
		mode = bg_mode_from_legacy(static_cast<int>(legacy_mode), has_wallpaper);
	}
	/* Migrate the old "wallpaper with special image value" representation. */
	if (mode == static_cast<int>(BgMode::Wallpaper)) {
		if (c->bg.image == kTransparentWallpaper || c->bg.image == kLocalImageMarker)
			mode = c->bg.image == kTransparentWallpaper
				       ? static_cast<int>(BgMode::Transparent)
				       : static_cast<int>(BgMode::LocalImage);
		else if (c->bg.image.empty() && !c->bg.image_file.empty())
			mode = static_cast<int>(BgMode::LocalImage);
	}
	c->bg.mode = std::clamp(mode, static_cast<int>(BgMode::Solid),
				static_cast<int>(BgMode::Transparent));

	if (!has_type && !obs_data_has_user_value(s, kBgGradientAngle))
		c->bg.gradient_angle =
			bg_gradient_angle_from_legacy(static_cast<int>(legacy_mode),
						      c->bg.gradient_angle);

	/* migration of the old 'bg_padding' key: only applies if the user never
	 * wrote the new keys (the defaults always exist, so has_key alone would
	 * never allow migrating). */
	if (!obs_data_has_user_value(s, kBgPaddingX) && !obs_data_has_user_value(s, kBgPaddingY) &&
	    obs_data_has_user_value(s, "bg_padding")) {
		c->bg.padding_x = obs_data_get_double(s, "bg_padding");
		c->bg.padding_y = c->bg.padding_x;
	}

	/* Migration from the single legacy 'click_sound_file' key. */
	if (!obs_data_has_user_value(s, kClickSoundFileLeft) &&
	    !obs_data_has_user_value(s, kClickSoundFileRight) &&
	    obs_data_has_user_value(s, kClickSoundFile)) {
		const char *old = obs_data_get_string(s, kClickSoundFile);
		if (old && *old) {
			c->click_sound.file_left = old;
			c->click_sound.file_right = old;
		}
	}
	/* Legacy per-side enable booleans predate the Mute list entries. */
	const bool legacy_left = read_bool(s, kClickSoundLeft, true);
	const bool legacy_right = read_bool(s, kClickSoundRight, true);
	if (!legacy_left)
		c->click_sound.file_left.clear();
	if (!legacy_right)
		c->click_sound.file_right.clear();
	c->click_sound.left = !c->click_sound.file_left.empty();
	c->click_sound.right = !c->click_sound.file_right.empty();

	/* Very old profiles only had the blur toggle; map it once when the new
	 * explicit mode key was never written. */
	if (!obs_data_has_user_value(s, kSpotlightOutsideMode) &&
	    c->spotlight.blur_enabled)
		c->spotlight.outside_mode = 1;

	sanitize_config(c);
}

void config_store(obs_data_t *s, const Config &c)
{
#define X(kind, key, member) TS_STORE_##kind(key, member);
	TS_SIMPLE_FIELDS(X)
#undef X
	obs_data_set_int(s, kBgType, c.bg.mode);
	obs_data_set_int(s, kBgMode, legacy_bg_mode_value(c.bg.mode));
}

} // namespace topscreen
