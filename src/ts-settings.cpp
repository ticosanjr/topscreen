#include "ts-settings.hpp"

#include "core/keys.hpp"

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen {

void config_defaults(obs_data_t *s)
{
	const Config def;

	obs_data_set_default_bool(s, kEffectsEnabled, def.effects_enabled);

	obs_data_set_default_bool(s, kBgEnabled, def.bg.enabled);
	obs_data_set_default_int(s, kBgMode, def.bg.mode);
	obs_data_set_default_int(s, kBgColorA, static_cast<long long>(def.bg.color_a));
	obs_data_set_default_int(s, kBgColorB, static_cast<long long>(def.bg.color_b));
	obs_data_set_default_string(s, kBgImage, def.bg.image.c_str());
	obs_data_set_default_string(s, kBgImageFile, def.bg.image_file.c_str());
	obs_data_set_default_double(s, kBgImageBlur, def.bg.image_blur);
	obs_data_set_default_int(s, kBgImageBlurType, def.bg.image_blur_type);
	obs_data_set_default_double(s, kBgPaddingX, def.bg.padding_x);
	obs_data_set_default_double(s, kBgPaddingY, def.bg.padding_y);
	obs_data_set_default_double(s, kBgCornerRadius, def.bg.corner_radius);
	obs_data_set_default_bool(s, kBgShadow, def.bg.shadow);
	obs_data_set_default_double(s, kBgShadowAlpha, def.bg.shadow_alpha);
	obs_data_set_default_double(s, kBgShadowBlur, def.bg.shadow_blur);
	obs_data_set_default_double(s, kBgShadowOffset, def.bg.shadow_offset);
	obs_data_set_default_bool(s, kBgKeepAspect, def.bg.keep_aspect);

	obs_data_set_default_bool(s, kCursorEnabled, def.cursor.enabled);
	obs_data_set_default_int(s, kCursorStyle, def.cursor.style);
	obs_data_set_default_double(s, kCursorSize, def.cursor.size);
	obs_data_set_default_bool(s, kCursorShadow, def.cursor.shadow);
	obs_data_set_default_double(s, kCursorSmooth, def.cursor.smooth);
	obs_data_set_default_double(s, kCursorSway, def.cursor.sway);
	obs_data_set_default_bool(s, kCursorHideIdle, def.cursor.hide_idle);
	obs_data_set_default_double(s, kCursorIdleDelayMs, def.cursor.idle_delay_ms);
	obs_data_set_default_bool(s, kCursorLoop, def.cursor.loop);
	obs_data_set_default_double(s, kCursorLoopMs, def.cursor.loop_ms);

	obs_data_set_default_int(s, kZoomMode, def.zoom.mode);
	obs_data_set_default_double(s, kZoomAmount, def.zoom.amount);
	obs_data_set_default_double(s, kZoomDurationMs, def.zoom.duration_ms);
	obs_data_set_default_double(s, kZoomIdleTimeoutMs, def.zoom.idle_timeout_ms);
	obs_data_set_default_double(s, kZoomPanAmount, def.zoom.pan_amount);
	obs_data_set_default_double(s, kZoomCenterAmount, def.zoom.center_amount);

	obs_data_set_default_int(s, kClickLeftEffect, def.click_left.effect);
	obs_data_set_default_double(s, kClickLeftSize, def.click_left.size);
	obs_data_set_default_int(s, kClickLeftColor, static_cast<long long>(def.click_left.color));
	obs_data_set_default_double(s, kClickLeftDurationMs, def.click_left.duration_ms);
	obs_data_set_default_int(s, kClickLeftEasing, def.click_left.easing);

	obs_data_set_default_int(s, kClickRightEffect, def.click_right.effect);
	obs_data_set_default_double(s, kClickRightSize, def.click_right.size);
	obs_data_set_default_int(s, kClickRightColor, static_cast<long long>(def.click_right.color));
	obs_data_set_default_double(s, kClickRightDurationMs, def.click_right.duration_ms);
	obs_data_set_default_int(s, kClickRightEasing, def.click_right.easing);

	obs_data_set_default_bool(s, kClickSoundEnabled, def.click_sound.enabled);
	obs_data_set_default_string(s, kClickSoundFileLeft, def.click_sound.file_left.c_str());
	obs_data_set_default_string(s, kClickSoundFileRight, def.click_sound.file_right.c_str());
	obs_data_set_default_double(s, kClickSoundVolume, def.click_sound.volume);
	obs_data_set_default_bool(s, kClickSoundLeft, def.click_sound.left);
	obs_data_set_default_bool(s, kClickSoundRight, def.click_sound.right);

	obs_data_set_default_bool(s, kMotionBlurEnabled, def.motion_blur.enabled);
	obs_data_set_default_double(s, kMotionBlurAmount, def.motion_blur.amount);

	obs_data_set_default_bool(s, kSpotlightEnabled, def.spotlight.enabled);
	obs_data_set_default_double(s, kSpotlightRadius, def.spotlight.radius);
	obs_data_set_default_double(s, kSpotlightDim, def.spotlight.dim);

	obs_data_set_default_bool(s, kKeysEnabled, def.keys.enabled);
	obs_data_set_default_bool(s, kKeysModifiersOnly, def.keys.modifiers_only);
	obs_data_set_default_double(s, kKeysTtlMs, def.keys.ttl_ms);
	obs_data_set_default_int(s, kKeysMax, def.keys.max_keys);
	obs_data_set_default_double(s, kKeysPosX, def.keys.pos_x);
	obs_data_set_default_double(s, kKeysPosY, def.keys.pos_y);
	obs_data_set_default_double(s, kKeysSize, def.keys.size);
	obs_data_set_default_string(s, kKeysFont, def.keys.font.c_str());
	obs_data_set_default_int(s, kKeysColor, static_cast<long long>(def.keys.color));
	obs_data_set_default_bool(s, kKeysShadow, def.keys.shadow);
	obs_data_set_default_bool(s, kKeysBand, def.keys.band);
	obs_data_set_default_int(s, kKeysBandColor, static_cast<long long>(def.keys.band_color));

	obs_data_set_default_bool(s, kHighlightEnabled, def.highlight.enabled);
	obs_data_set_default_double(s, kHighlightHoldMs, def.highlight.hold_ms);
	obs_data_set_default_double(s, kHighlightFadeMs, def.highlight.fade_ms);
	obs_data_set_default_int(s, kHighlightColor, static_cast<long long>(def.highlight.color));

	/* Active profile (slot index). Default does not become a "user value", so
	 * a new source keeps being treated as "no profile chosen". */
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

void config_load(obs_data_t *s, Config *c)
{
	const Config d;

	c->effects_enabled = read_bool(s, kEffectsEnabled, d.effects_enabled);

	c->bg.enabled = read_bool(s, kBgEnabled, d.bg.enabled);
	c->bg.mode = static_cast<int>(read_int(s, kBgMode, d.bg.mode));
	c->bg.color_a = static_cast<uint32_t>(read_int(s, kBgColorA, d.bg.color_a));
	c->bg.color_b = static_cast<uint32_t>(read_int(s, kBgColorB, d.bg.color_b));
	c->bg.image = read_string(s, kBgImage, d.bg.image);
	c->bg.image_file = read_string(s, kBgImageFile, d.bg.image_file);
	c->bg.image_blur = read_double(s, kBgImageBlur, d.bg.image_blur);
	c->bg.image_blur_type = static_cast<int>(read_int(s, kBgImageBlurType, d.bg.image_blur_type));
	c->bg.padding_x = read_double(s, kBgPaddingX, d.bg.padding_x);
	c->bg.padding_y = read_double(s, kBgPaddingY, d.bg.padding_y);
	/* migration of the old 'bg_padding' key: only applies if the user never
	 * wrote the new keys (the defaults always exist, so has_key
	 * alone would never allow migrating). */
	if (!obs_data_has_user_value(s, kBgPaddingX) && !obs_data_has_user_value(s, kBgPaddingY) &&
	    obs_data_has_user_value(s, "bg_padding")) {
		c->bg.padding_x = obs_data_get_double(s, "bg_padding");
		c->bg.padding_y = c->bg.padding_x;
	}
	c->bg.corner_radius = read_double(s, kBgCornerRadius, d.bg.corner_radius);
	c->bg.shadow = read_bool(s, kBgShadow, d.bg.shadow);
	c->bg.shadow_alpha = read_double(s, kBgShadowAlpha, d.bg.shadow_alpha);
	c->bg.shadow_blur = read_double(s, kBgShadowBlur, d.bg.shadow_blur);
	c->bg.shadow_offset = read_double(s, kBgShadowOffset, d.bg.shadow_offset);
	c->bg.keep_aspect = read_bool(s, kBgKeepAspect, d.bg.keep_aspect);

	c->cursor.enabled = read_bool(s, kCursorEnabled, d.cursor.enabled);
	c->cursor.style = static_cast<int>(read_int(s, kCursorStyle, d.cursor.style));
	c->cursor.size = read_double(s, kCursorSize, d.cursor.size);
	c->cursor.shadow = read_bool(s, kCursorShadow, d.cursor.shadow);
	c->cursor.smooth = read_double(s, kCursorSmooth, d.cursor.smooth);
	c->cursor.sway = read_double(s, kCursorSway, d.cursor.sway);
	c->cursor.hide_idle = read_bool(s, kCursorHideIdle, d.cursor.hide_idle);
	c->cursor.idle_delay_ms = read_double(s, kCursorIdleDelayMs, d.cursor.idle_delay_ms);
	c->cursor.loop = read_bool(s, kCursorLoop, d.cursor.loop);
	c->cursor.loop_ms = read_double(s, kCursorLoopMs, d.cursor.loop_ms);

	c->zoom.mode = static_cast<int>(read_int(s, kZoomMode, d.zoom.mode));
	c->zoom.amount = read_double(s, kZoomAmount, d.zoom.amount);
	c->zoom.duration_ms = read_double(s, kZoomDurationMs, d.zoom.duration_ms);
	c->zoom.idle_timeout_ms = read_double(s, kZoomIdleTimeoutMs, d.zoom.idle_timeout_ms);
	c->zoom.pan_amount = read_double(s, kZoomPanAmount, d.zoom.pan_amount);
	c->zoom.center_amount = read_double(s, kZoomCenterAmount, d.zoom.center_amount);

	c->click_left.effect = static_cast<int>(read_int(s, kClickLeftEffect, d.click_left.effect));
	c->click_left.size = read_double(s, kClickLeftSize, d.click_left.size);
	c->click_left.color = static_cast<uint32_t>(read_int(s, kClickLeftColor, d.click_left.color));
	c->click_left.duration_ms = read_double(s, kClickLeftDurationMs, d.click_left.duration_ms);
	c->click_left.easing = static_cast<int>(read_int(s, kClickLeftEasing, d.click_left.easing));

	c->click_right.effect = static_cast<int>(read_int(s, kClickRightEffect, d.click_right.effect));
	c->click_right.size = read_double(s, kClickRightSize, d.click_right.size);
	c->click_right.color = static_cast<uint32_t>(read_int(s, kClickRightColor, d.click_right.color));
	c->click_right.duration_ms = read_double(s, kClickRightDurationMs, d.click_right.duration_ms);
	c->click_right.easing = static_cast<int>(read_int(s, kClickRightEasing, d.click_right.easing));

	c->click_sound.enabled = read_bool(s, kClickSoundEnabled, d.click_sound.enabled);
	c->click_sound.file_left = read_string(s, kClickSoundFileLeft, d.click_sound.file_left);
	c->click_sound.file_right = read_string(s, kClickSoundFileRight, d.click_sound.file_right);
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
	c->click_sound.volume = read_double(s, kClickSoundVolume, d.click_sound.volume);
	c->click_sound.left = read_bool(s, kClickSoundLeft, d.click_sound.left);
	c->click_sound.right = read_bool(s, kClickSoundRight, d.click_sound.right);

	c->motion_blur.enabled = read_bool(s, kMotionBlurEnabled, d.motion_blur.enabled);
	c->motion_blur.amount = read_double(s, kMotionBlurAmount, d.motion_blur.amount);

	c->spotlight.enabled = read_bool(s, kSpotlightEnabled, d.spotlight.enabled);
	c->spotlight.radius = read_double(s, kSpotlightRadius, d.spotlight.radius);
	c->spotlight.dim = read_double(s, kSpotlightDim, d.spotlight.dim);

	c->keys.enabled = read_bool(s, kKeysEnabled, d.keys.enabled);
	c->keys.modifiers_only = read_bool(s, kKeysModifiersOnly, d.keys.modifiers_only);
	c->keys.ttl_ms = read_double(s, kKeysTtlMs, d.keys.ttl_ms);
	c->keys.max_keys = static_cast<int>(read_int(s, kKeysMax, d.keys.max_keys));
	c->keys.pos_x = read_double(s, kKeysPosX, d.keys.pos_x);
	c->keys.pos_y = read_double(s, kKeysPosY, d.keys.pos_y);
	c->keys.size = read_double(s, kKeysSize, d.keys.size);
	c->keys.font = read_string(s, kKeysFont, d.keys.font);
	c->keys.color = static_cast<uint32_t>(read_int(s, kKeysColor, d.keys.color));
	c->keys.shadow = read_bool(s, kKeysShadow, d.keys.shadow);
	c->keys.band = read_bool(s, kKeysBand, d.keys.band);
	c->keys.band_color = static_cast<uint32_t>(read_int(s, kKeysBandColor, d.keys.band_color));

	c->highlight.enabled = read_bool(s, kHighlightEnabled, d.highlight.enabled);
	c->highlight.hold_ms = read_double(s, kHighlightHoldMs, d.highlight.hold_ms);
	c->highlight.fade_ms = read_double(s, kHighlightFadeMs, d.highlight.fade_ms);
	c->highlight.color = static_cast<uint32_t>(read_int(s, kHighlightColor, d.highlight.color));

	/* Migration: 0.7/0.3 were the old cursor defaults (with "slide"). */
	if (c->cursor.smooth == 0.7 && c->cursor.sway == 0.3) {
		c->cursor.smooth = 0.0;
		c->cursor.sway = 0.0;
	}
}

void config_store(obs_data_t *s, const Config &c)
{
	obs_data_set_bool(s, kEffectsEnabled, c.effects_enabled);

	obs_data_set_bool(s, kBgEnabled, c.bg.enabled);
	obs_data_set_int(s, kBgMode, c.bg.mode);
	obs_data_set_int(s, kBgColorA, static_cast<long long>(c.bg.color_a));
	obs_data_set_int(s, kBgColorB, static_cast<long long>(c.bg.color_b));
	obs_data_set_string(s, kBgImage, c.bg.image.c_str());
	obs_data_set_string(s, kBgImageFile, c.bg.image_file.c_str());
	obs_data_set_double(s, kBgImageBlur, c.bg.image_blur);
	obs_data_set_int(s, kBgImageBlurType, c.bg.image_blur_type);
	obs_data_set_double(s, kBgPaddingX, c.bg.padding_x);
	obs_data_set_double(s, kBgPaddingY, c.bg.padding_y);
	obs_data_set_double(s, kBgCornerRadius, c.bg.corner_radius);
	obs_data_set_bool(s, kBgShadow, c.bg.shadow);
	obs_data_set_double(s, kBgShadowAlpha, c.bg.shadow_alpha);
	obs_data_set_double(s, kBgShadowBlur, c.bg.shadow_blur);
	obs_data_set_double(s, kBgShadowOffset, c.bg.shadow_offset);
	obs_data_set_bool(s, kBgKeepAspect, c.bg.keep_aspect);

	obs_data_set_bool(s, kCursorEnabled, c.cursor.enabled);
	obs_data_set_int(s, kCursorStyle, c.cursor.style);
	obs_data_set_double(s, kCursorSize, c.cursor.size);
	obs_data_set_bool(s, kCursorShadow, c.cursor.shadow);
	obs_data_set_double(s, kCursorSmooth, c.cursor.smooth);
	obs_data_set_double(s, kCursorSway, c.cursor.sway);
	obs_data_set_bool(s, kCursorHideIdle, c.cursor.hide_idle);
	obs_data_set_double(s, kCursorIdleDelayMs, c.cursor.idle_delay_ms);
	obs_data_set_bool(s, kCursorLoop, c.cursor.loop);
	obs_data_set_double(s, kCursorLoopMs, c.cursor.loop_ms);

	obs_data_set_int(s, kZoomMode, c.zoom.mode);
	obs_data_set_double(s, kZoomAmount, c.zoom.amount);
	obs_data_set_double(s, kZoomDurationMs, c.zoom.duration_ms);
	obs_data_set_double(s, kZoomIdleTimeoutMs, c.zoom.idle_timeout_ms);
	obs_data_set_double(s, kZoomPanAmount, c.zoom.pan_amount);
	obs_data_set_double(s, kZoomCenterAmount, c.zoom.center_amount);

	obs_data_set_int(s, kClickLeftEffect, c.click_left.effect);
	obs_data_set_double(s, kClickLeftSize, c.click_left.size);
	obs_data_set_int(s, kClickLeftColor, static_cast<long long>(c.click_left.color));
	obs_data_set_double(s, kClickLeftDurationMs, c.click_left.duration_ms);
	obs_data_set_int(s, kClickLeftEasing, c.click_left.easing);

	obs_data_set_int(s, kClickRightEffect, c.click_right.effect);
	obs_data_set_double(s, kClickRightSize, c.click_right.size);
	obs_data_set_int(s, kClickRightColor, static_cast<long long>(c.click_right.color));
	obs_data_set_double(s, kClickRightDurationMs, c.click_right.duration_ms);
	obs_data_set_int(s, kClickRightEasing, c.click_right.easing);

	obs_data_set_bool(s, kClickSoundEnabled, c.click_sound.enabled);
	obs_data_set_string(s, kClickSoundFileLeft, c.click_sound.file_left.c_str());
	obs_data_set_string(s, kClickSoundFileRight, c.click_sound.file_right.c_str());
	obs_data_set_double(s, kClickSoundVolume, c.click_sound.volume);
	obs_data_set_bool(s, kClickSoundLeft, c.click_sound.left);
	obs_data_set_bool(s, kClickSoundRight, c.click_sound.right);

	obs_data_set_bool(s, kMotionBlurEnabled, c.motion_blur.enabled);
	obs_data_set_double(s, kMotionBlurAmount, c.motion_blur.amount);

	obs_data_set_bool(s, kSpotlightEnabled, c.spotlight.enabled);
	obs_data_set_double(s, kSpotlightRadius, c.spotlight.radius);
	obs_data_set_double(s, kSpotlightDim, c.spotlight.dim);

	obs_data_set_bool(s, kKeysEnabled, c.keys.enabled);
	obs_data_set_bool(s, kKeysModifiersOnly, c.keys.modifiers_only);
	obs_data_set_double(s, kKeysTtlMs, c.keys.ttl_ms);
	obs_data_set_int(s, kKeysMax, c.keys.max_keys);
	obs_data_set_double(s, kKeysPosX, c.keys.pos_x);
	obs_data_set_double(s, kKeysPosY, c.keys.pos_y);
	obs_data_set_double(s, kKeysSize, c.keys.size);
	obs_data_set_string(s, kKeysFont, c.keys.font.c_str());
	obs_data_set_int(s, kKeysColor, static_cast<long long>(c.keys.color));
	obs_data_set_bool(s, kKeysShadow, c.keys.shadow);
	obs_data_set_bool(s, kKeysBand, c.keys.band);
	obs_data_set_int(s, kKeysBandColor, static_cast<long long>(c.keys.band_color));

	obs_data_set_bool(s, kHighlightEnabled, c.highlight.enabled);
	obs_data_set_double(s, kHighlightHoldMs, c.highlight.hold_ms);
	obs_data_set_double(s, kHighlightFadeMs, c.highlight.fade_ms);
	obs_data_set_int(s, kHighlightColor, static_cast<long long>(c.highlight.color));
}

} // namespace topscreen
