#include "preset.hpp"

#include "cursor_catalog.hpp"
#include "keys.hpp"

namespace topscreen::core {

namespace {

const json::Value *lookup(const json::Object &o, const char *key)
{
	auto it = o.find(key);
	return it == o.end() ? nullptr : &it->second;
}

double read_number(const json::Object &o, const char *key, double fallback)
{
	const json::Value *v = lookup(o, key);
	return (v && v->is_number()) ? v->number : fallback;
}

bool read_bool(const json::Object &o, const char *key, bool fallback)
{
	const json::Value *v = lookup(o, key);
	return (v && v->is_bool()) ? v->boolean : fallback;
}

int read_int(const json::Object &o, const char *key, int fallback)
{
	const json::Value *v = lookup(o, key);
	return (v && v->is_number()) ? static_cast<int>(v->number) : fallback;
}

std::string read_string(const json::Object &o, const char *key, const std::string &fallback)
{
	const json::Value *v = lookup(o, key);
	return (v && v->is_string()) ? v->str : fallback;
}

} // namespace

bool Preset::from_json(const std::string &text, Preset *out, std::string *error)
{
	json::Value root;
	if (!json::parse(text, &root, error))
		return false;

	if (!root.is_object()) {
		if (error)
			*error = "preset: root is not an object";
		return false;
	}

	const json::Value *name = root.find("name");
	if (name && name->is_string())
		out->name = name->str;

	const json::Value *desc = root.find("description");
	if (desc && desc->is_string())
		out->description = desc->str;

	const json::Value *settings = root.find("settings");
	if (!settings || !settings->is_object()) {
		if (error)
			*error = "preset: missing the 'settings' object";
		return false;
	}

	out->settings_ = *settings;
	return true;
}

bool Preset::apply(Config *cfg, std::string *error) const
{
	if (!settings_.is_object()) {
		if (error)
			*error = "preset: 'settings' is not an object";
		return false;
	}

	const auto &o = *settings_.object;
	using namespace keys;

	cfg->effects_enabled = read_bool(o, kEffectsEnabled, cfg->effects_enabled);

	/* Background */
	cfg->bg.enabled = read_bool(o, kBgEnabled, cfg->bg.enabled);
	cfg->bg.mode = read_int(o, kBgMode, cfg->bg.mode);
	cfg->bg.color_a = static_cast<uint32_t>(read_number(o, kBgColorA, cfg->bg.color_a));
	cfg->bg.color_b = static_cast<uint32_t>(read_number(o, kBgColorB, cfg->bg.color_b));
	cfg->bg.image = read_string(o, kBgImage, cfg->bg.image);
	cfg->bg.image_file = read_string(o, kBgImageFile, cfg->bg.image_file);
	cfg->bg.image_blur = read_number(o, kBgImageBlur, cfg->bg.image_blur);
	cfg->bg.image_blur_type = read_int(o, kBgImageBlurType, cfg->bg.image_blur_type);
	cfg->bg.padding_x = read_number(o, kBgPaddingX, cfg->bg.padding_x);
	cfg->bg.padding_y = read_number(o, kBgPaddingY, cfg->bg.padding_y);
	cfg->bg.corner_radius = read_number(o, kBgCornerRadius, cfg->bg.corner_radius);
	cfg->bg.shadow = read_bool(o, kBgShadow, cfg->bg.shadow);
	cfg->bg.shadow_alpha = read_number(o, kBgShadowAlpha, cfg->bg.shadow_alpha);
	cfg->bg.shadow_blur = read_number(o, kBgShadowBlur, cfg->bg.shadow_blur);
	cfg->bg.shadow_offset_x = read_number(o, kBgShadowOffsetX, cfg->bg.shadow_offset_x);
	cfg->bg.shadow_offset = read_number(o, kBgShadowOffset, cfg->bg.shadow_offset);
	cfg->bg.keep_aspect = read_bool(o, kBgKeepAspect, cfg->bg.keep_aspect);

	/* Cursor */
	cfg->cursor.enabled = read_bool(o, kCursorEnabled, cfg->cursor.enabled);
	cfg->cursor.style = cursor_style_clamp(read_int(o, kCursorStyle, cfg->cursor.style));
	cfg->cursor.size = read_number(o, kCursorSize, cfg->cursor.size);
	cfg->cursor.shadow = read_bool(o, kCursorShadow, cfg->cursor.shadow);
	cfg->cursor.smooth = read_number(o, kCursorSmooth, cfg->cursor.smooth);
	cfg->cursor.sway = read_number(o, kCursorSway, cfg->cursor.sway);
	cfg->cursor.hide_idle = read_bool(o, kCursorHideIdle, cfg->cursor.hide_idle);
	cfg->cursor.idle_delay_ms = read_number(o, kCursorIdleDelayMs, cfg->cursor.idle_delay_ms);
	cfg->cursor.loop = read_bool(o, kCursorLoop, cfg->cursor.loop);
	cfg->cursor.loop_ms = read_number(o, kCursorLoopMs, cfg->cursor.loop_ms);

	/* Zoom */
	cfg->zoom.mode = read_int(o, kZoomMode, cfg->zoom.mode);
	cfg->zoom.amount = read_number(o, kZoomAmount, cfg->zoom.amount);
	cfg->zoom.duration_ms = read_number(o, kZoomDurationMs, cfg->zoom.duration_ms);
	cfg->zoom.idle_timeout_ms = read_number(o, kZoomIdleTimeoutMs, cfg->zoom.idle_timeout_ms);
	cfg->zoom.pan_amount = read_number(o, kZoomPanAmount, cfg->zoom.pan_amount);
	cfg->zoom.center_amount = read_number(o, kZoomCenterAmount, cfg->zoom.center_amount);

	/* Left click */
	cfg->click_left.effect = read_int(o, kClickLeftEffect, cfg->click_left.effect);
	cfg->click_left.size = read_number(o, kClickLeftSize, cfg->click_left.size);
	cfg->click_left.color = static_cast<uint32_t>(read_number(o, kClickLeftColor, cfg->click_left.color));
	cfg->click_left.duration_ms = read_number(o, kClickLeftDurationMs, cfg->click_left.duration_ms);
	cfg->click_left.easing = read_int(o, kClickLeftEasing, cfg->click_left.easing);

	/* Right click */
	cfg->click_right.effect = read_int(o, kClickRightEffect, cfg->click_right.effect);
	cfg->click_right.size = read_number(o, kClickRightSize, cfg->click_right.size);
	cfg->click_right.color =
		static_cast<uint32_t>(read_number(o, kClickRightColor, cfg->click_right.color));
	cfg->click_right.duration_ms = read_number(o, kClickRightDurationMs, cfg->click_right.duration_ms);
	cfg->click_right.easing = read_int(o, kClickRightEasing, cfg->click_right.easing);

	/* Click sound */
	cfg->click_sound.enabled = read_bool(o, kClickSoundEnabled, cfg->click_sound.enabled);
	cfg->click_sound.file_left = read_string(o, kClickSoundFileLeft, cfg->click_sound.file_left);
	cfg->click_sound.file_right =
		read_string(o, kClickSoundFileRight, cfg->click_sound.file_right);
	/* Legacy single-file presets apply to both buttons. */
	if (o.find(kClickSoundFileLeft) == o.end() && o.find(kClickSoundFileRight) == o.end() &&
	    o.find(kClickSoundFile) != o.end()) {
		const std::string old = read_string(o, kClickSoundFile, "");
		if (!old.empty()) {
			cfg->click_sound.file_left = old;
			cfg->click_sound.file_right = old;
		}
	}
	cfg->click_sound.volume = read_number(o, kClickSoundVolume, cfg->click_sound.volume);
	cfg->click_sound.left = read_bool(o, kClickSoundLeft, cfg->click_sound.left);
	cfg->click_sound.right = read_bool(o, kClickSoundRight, cfg->click_sound.right);

	/* Motion blur */
	cfg->motion_blur.enabled = read_bool(o, kMotionBlurEnabled, cfg->motion_blur.enabled);
	cfg->motion_blur.amount = read_number(o, kMotionBlurAmount, cfg->motion_blur.amount);

	/* Spotlight */
	cfg->spotlight.enabled = read_bool(o, kSpotlightEnabled, cfg->spotlight.enabled);
	cfg->spotlight.radius = read_number(o, kSpotlightRadius, cfg->spotlight.radius);
	cfg->spotlight.dim = read_number(o, kSpotlightDim, cfg->spotlight.dim);
	cfg->spotlight.shape = static_cast<int>(read_number(o, kSpotlightShape, cfg->spotlight.shape));
	cfg->spotlight.corner_radius =
		read_number(o, kSpotlightCornerRadius, cfg->spotlight.corner_radius);
	cfg->spotlight.border_enabled =
		read_bool(o, kSpotlightBorderEnabled, cfg->spotlight.border_enabled);
	cfg->spotlight.border_color = static_cast<uint32_t>(
		read_number(o, kSpotlightBorderColor, cfg->spotlight.border_color));
	cfg->spotlight.border_width =
		read_number(o, kSpotlightBorderWidth, cfg->spotlight.border_width);
	cfg->spotlight.shadow_enabled =
		read_bool(o, kSpotlightShadowEnabled, cfg->spotlight.shadow_enabled);
	cfg->spotlight.shadow_color = static_cast<uint32_t>(
		read_number(o, kSpotlightShadowColor, cfg->spotlight.shadow_color));
	cfg->spotlight.shadow_alpha =
		read_number(o, kSpotlightShadowAlpha, cfg->spotlight.shadow_alpha);
	cfg->spotlight.shadow_blur =
		read_number(o, kSpotlightShadowBlur, cfg->spotlight.shadow_blur);
	cfg->spotlight.shadow_offset_x =
		read_number(o, kSpotlightShadowOffsetX, cfg->spotlight.shadow_offset_x);
	cfg->spotlight.shadow_offset_y =
		read_number(o, kSpotlightShadowOffsetY, cfg->spotlight.shadow_offset_y);
	cfg->spotlight.blur_enabled =
		read_bool(o, kSpotlightBlurEnabled, cfg->spotlight.blur_enabled);
	cfg->spotlight.blur_amount =
		read_number(o, kSpotlightBlurAmount, cfg->spotlight.blur_amount);
	cfg->spotlight.zoom_amount =
		read_number(o, kSpotlightZoomAmount, cfg->spotlight.zoom_amount);
	cfg->spotlight.in_ms = read_number(o, kSpotlightInMs, cfg->spotlight.in_ms);
	cfg->spotlight.out_ms = read_number(o, kSpotlightOutMs, cfg->spotlight.out_ms);
	cfg->spotlight.drop_px = read_number(o, kSpotlightDropPx, cfg->spotlight.drop_px);

	/* Keys */
	cfg->keys.enabled = read_bool(o, kKeysEnabled, cfg->keys.enabled);
	cfg->keys.modifiers_only = read_bool(o, kKeysModifiersOnly, cfg->keys.modifiers_only);
	cfg->keys.show_special = read_bool(o, kKeysShowSpecial, cfg->keys.show_special);
	cfg->keys.ttl_ms = read_number(o, kKeysTtlMs, cfg->keys.ttl_ms);
	cfg->keys.max_keys = read_int(o, kKeysMax, cfg->keys.max_keys);
	cfg->keys.pos_x = read_number(o, kKeysPosX, cfg->keys.pos_x);
	cfg->keys.pos_y = read_number(o, kKeysPosY, cfg->keys.pos_y);
	cfg->keys.size = read_number(o, kKeysSize, cfg->keys.size);
	cfg->keys.font = read_string(o, kKeysFont, cfg->keys.font);
	cfg->keys.color = static_cast<uint32_t>(read_number(o, kKeysColor, cfg->keys.color));
	cfg->keys.shadow = read_bool(o, kKeysShadow, cfg->keys.shadow);
	cfg->keys.band = read_bool(o, kKeysBand, cfg->keys.band);
	cfg->keys.band_color = static_cast<uint32_t>(read_number(o, kKeysBandColor, cfg->keys.band_color));

	/* Highlighter */
	cfg->highlight.enabled = read_bool(o, kHighlightEnabled, cfg->highlight.enabled);
	cfg->highlight.hotkey_enabled =
		read_bool(o, kHighlightHotkeyEnabled, cfg->highlight.hotkey_enabled);
	cfg->highlight.hold_ms = read_number(o, kHighlightHoldMs, cfg->highlight.hold_ms);
	cfg->highlight.fade_ms = read_number(o, kHighlightFadeMs, cfg->highlight.fade_ms);
	cfg->highlight.color = static_cast<uint32_t>(read_number(o, kHighlightColor, cfg->highlight.color));

	cfg->highlight.mode = read_int(o, kHighlightMode, cfg->highlight.mode);
	cfg->highlight.thickness = read_number(o, kHighlightThickness, cfg->highlight.thickness);
	cfg->highlight.zoom_dir = read_int(o, kHighlightZoomDir, cfg->highlight.zoom_dir);
	cfg->highlight.zoom_delay_ms = read_number(o, kHighlightZoomDelayMs, cfg->highlight.zoom_delay_ms);
	cfg->highlight.zoom_max = read_number(o, kHighlightZoomMax, cfg->highlight.zoom_max);
	cfg->highlight.zoom_in_ms = read_number(o, kHighlightZoomInMs, cfg->highlight.zoom_in_ms);
	cfg->highlight.zoom_hold_ms = read_number(o, kHighlightZoomHoldMs, cfg->highlight.zoom_hold_ms);
	cfg->highlight.zoom_out_ms = read_number(o, kHighlightZoomOutMs, cfg->highlight.zoom_out_ms);
	cfg->highlight.press_delay_ms = read_number(o, kHighlightPressDelayMs, cfg->highlight.press_delay_ms);

	cfg->monitor_manual = read_bool(o, kMonitorManual, cfg->monitor_manual);
	cfg->monitor_x = static_cast<int>(read_int(o, kMonitorX, cfg->monitor_x));
	cfg->monitor_y = static_cast<int>(read_int(o, kMonitorY, cfg->monitor_y));

	return true;
}

} // namespace topscreen::core
