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
	obs_data_set_default_double(s, kBgPadding, def.bg.padding);
	obs_data_set_default_double(s, kBgCornerRadius, def.bg.corner_radius);
	obs_data_set_default_bool(s, kBgShadow, def.bg.shadow);
	obs_data_set_default_double(s, kBgShadowAlpha, def.bg.shadow_alpha);
	obs_data_set_default_double(s, kBgShadowBlur, def.bg.shadow_blur);

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
	obs_data_set_default_string(s, kClickSoundFile, def.click_sound.file.c_str());
	obs_data_set_default_double(s, kClickSoundVolume, def.click_sound.volume);
	obs_data_set_default_bool(s, kClickSoundLeft, def.click_sound.left);
	obs_data_set_default_bool(s, kClickSoundRight, def.click_sound.right);
}

void config_load(obs_data_t *s, Config *c)
{
	c->effects_enabled = obs_data_get_bool(s, kEffectsEnabled);

	c->bg.enabled = obs_data_get_bool(s, kBgEnabled);
	c->bg.mode = static_cast<int>(obs_data_get_int(s, kBgMode));
	c->bg.color_a = static_cast<uint32_t>(obs_data_get_int(s, kBgColorA));
	c->bg.color_b = static_cast<uint32_t>(obs_data_get_int(s, kBgColorB));
	c->bg.padding = obs_data_get_double(s, kBgPadding);
	c->bg.corner_radius = obs_data_get_double(s, kBgCornerRadius);
	c->bg.shadow = obs_data_get_bool(s, kBgShadow);
	c->bg.shadow_alpha = obs_data_get_double(s, kBgShadowAlpha);
	c->bg.shadow_blur = obs_data_get_double(s, kBgShadowBlur);

	c->cursor.enabled = obs_data_get_bool(s, kCursorEnabled);
	c->cursor.style = static_cast<int>(obs_data_get_int(s, kCursorStyle));
	c->cursor.size = obs_data_get_double(s, kCursorSize);
	c->cursor.shadow = obs_data_get_bool(s, kCursorShadow);
	c->cursor.smooth = obs_data_get_double(s, kCursorSmooth);
	c->cursor.sway = obs_data_get_double(s, kCursorSway);
	c->cursor.hide_idle = obs_data_get_bool(s, kCursorHideIdle);
	c->cursor.idle_delay_ms = obs_data_get_double(s, kCursorIdleDelayMs);
	c->cursor.loop = obs_data_get_bool(s, kCursorLoop);
	c->cursor.loop_ms = obs_data_get_double(s, kCursorLoopMs);

	c->zoom.mode = static_cast<int>(obs_data_get_int(s, kZoomMode));
	c->zoom.amount = obs_data_get_double(s, kZoomAmount);
	c->zoom.duration_ms = obs_data_get_double(s, kZoomDurationMs);
	c->zoom.idle_timeout_ms = obs_data_get_double(s, kZoomIdleTimeoutMs);
	c->zoom.pan_amount = obs_data_get_double(s, kZoomPanAmount);

	c->click_left.effect = static_cast<int>(obs_data_get_int(s, kClickLeftEffect));
	c->click_left.size = obs_data_get_double(s, kClickLeftSize);
	c->click_left.color = static_cast<uint32_t>(obs_data_get_int(s, kClickLeftColor));
	c->click_left.duration_ms = obs_data_get_double(s, kClickLeftDurationMs);
	c->click_left.easing = static_cast<int>(obs_data_get_int(s, kClickLeftEasing));

	c->click_right.effect = static_cast<int>(obs_data_get_int(s, kClickRightEffect));
	c->click_right.size = obs_data_get_double(s, kClickRightSize);
	c->click_right.color = static_cast<uint32_t>(obs_data_get_int(s, kClickRightColor));
	c->click_right.duration_ms = obs_data_get_double(s, kClickRightDurationMs);
	c->click_right.easing = static_cast<int>(obs_data_get_int(s, kClickRightEasing));

	c->click_sound.enabled = obs_data_get_bool(s, kClickSoundEnabled);
	const char *file = obs_data_get_string(s, kClickSoundFile);
	c->click_sound.file = file ? file : "click-soft.wav";
	c->click_sound.volume = obs_data_get_double(s, kClickSoundVolume);
	c->click_sound.left = obs_data_get_bool(s, kClickSoundLeft);
	c->click_sound.right = obs_data_get_bool(s, kClickSoundRight);
}

void config_store(obs_data_t *s, const Config &c)
{
	obs_data_set_bool(s, kEffectsEnabled, c.effects_enabled);

	obs_data_set_bool(s, kBgEnabled, c.bg.enabled);
	obs_data_set_int(s, kBgMode, c.bg.mode);
	obs_data_set_int(s, kBgColorA, static_cast<long long>(c.bg.color_a));
	obs_data_set_int(s, kBgColorB, static_cast<long long>(c.bg.color_b));
	obs_data_set_double(s, kBgPadding, c.bg.padding);
	obs_data_set_double(s, kBgCornerRadius, c.bg.corner_radius);
	obs_data_set_bool(s, kBgShadow, c.bg.shadow);
	obs_data_set_double(s, kBgShadowAlpha, c.bg.shadow_alpha);
	obs_data_set_double(s, kBgShadowBlur, c.bg.shadow_blur);

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
	obs_data_set_string(s, kClickSoundFile, c.click_sound.file.c_str());
	obs_data_set_double(s, kClickSoundVolume, c.click_sound.volume);
	obs_data_set_bool(s, kClickSoundLeft, c.click_sound.left);
	obs_data_set_bool(s, kClickSoundRight, c.click_sound.right);
}

} // namespace topscreen
