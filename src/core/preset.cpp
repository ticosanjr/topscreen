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
	if (lookup(o, kBgType)) {
		cfg->bg.mode = read_int(o, kBgType, cfg->bg.mode);
	} else {
		const int legacy = read_int(o, kBgMode, cfg->bg.mode);
		const bool has_wallpaper = !read_string(o, kBgImage, cfg->bg.image).empty() ||
					   !read_string(o, kBgImageFile, cfg->bg.image_file).empty();
		cfg->bg.mode = bg_mode_from_legacy(legacy, has_wallpaper);
		if (!lookup(o, kBgGradientAngle))
			cfg->bg.gradient_angle =
				bg_gradient_angle_from_legacy(legacy, cfg->bg.gradient_angle);
	}
	if (cfg->bg.mode < 0 || cfg->bg.mode > static_cast<int>(BgMode::Transparent))
		cfg->bg.mode = static_cast<int>(BgMode::Gradient);
	cfg->bg.color_a = static_cast<uint32_t>(read_number(o, kBgColorA, cfg->bg.color_a));
	cfg->bg.color_b = static_cast<uint32_t>(read_number(o, kBgColorB, cfg->bg.color_b));
	cfg->bg.gradient_angle = read_number(o, kBgGradientAngle, cfg->bg.gradient_angle);
	cfg->bg.gradient_center_x =
		read_number(o, kBgGradientCenterX, cfg->bg.gradient_center_x);
	cfg->bg.gradient_center_y =
		read_number(o, kBgGradientCenterY, cfg->bg.gradient_center_y);
	cfg->bg.image = read_string(o, kBgImage, cfg->bg.image);
	cfg->bg.image_file = read_string(o, kBgImageFile, cfg->bg.image_file);
	if (cfg->bg.mode == static_cast<int>(BgMode::Wallpaper)) {
		if (cfg->bg.image == "__transparent__")
			cfg->bg.mode = static_cast<int>(BgMode::Transparent);
		else if (cfg->bg.image == "__local__")
			cfg->bg.mode = static_cast<int>(BgMode::LocalImage);
		else if (cfg->bg.image.empty() && !cfg->bg.image_file.empty())
			cfg->bg.mode = static_cast<int>(BgMode::LocalImage);
	}
	cfg->bg.image_blur = read_number(o, kBgImageBlur, cfg->bg.image_blur);
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
	cfg->cursor.secondary_style = read_int(o, kCursorSecondaryStyle, cfg->cursor.secondary_style);
	cfg->cursor.link_detection =
		read_bool(o, kCursorLinkDetection, cfg->cursor.link_detection);
	cfg->cursor.size = read_number(o, kCursorSize, cfg->cursor.size);
	cfg->cursor.rotation_offset_deg =
		read_number(o, kCursorRotationOffset, cfg->cursor.rotation_offset_deg);
	cfg->cursor.shadow = read_bool(o, kCursorShadow, cfg->cursor.shadow);
	cfg->cursor.follow_mode = read_int(o, kCursorFollowMode, cfg->cursor.follow_mode);
	cfg->cursor.glide_ms = read_number(o, kCursorGlideMs, cfg->cursor.glide_ms);
	cfg->cursor.hide_idle = read_bool(o, kCursorHideIdle, cfg->cursor.hide_idle);
	cfg->cursor.idle_delay_ms = read_number(o, kCursorIdleDelayMs, cfg->cursor.idle_delay_ms);

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
	/* Legacy per-side booleans: false meant muted before the Mute list items. */
	if (!read_bool(o, kClickSoundLeft, true))
		cfg->click_sound.file_left.clear();
	if (!read_bool(o, kClickSoundRight, true))
		cfg->click_sound.file_right.clear();
	cfg->click_sound.left = !cfg->click_sound.file_left.empty();
	cfg->click_sound.right = !cfg->click_sound.file_right.empty();

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
	cfg->spotlight.blur_enabled =
		read_bool(o, kSpotlightBlurEnabled, cfg->spotlight.blur_enabled);
	cfg->spotlight.outside_mode = read_int(o, kSpotlightOutsideMode, cfg->spotlight.outside_mode);
	/* Presets written before the explicit mode only had blur_enabled. */
	if (!lookup(o, kSpotlightOutsideMode) && cfg->spotlight.blur_enabled)
		cfg->spotlight.outside_mode = 1;
	cfg->spotlight.blur_amount =
		read_number(o, kSpotlightBlurAmount, cfg->spotlight.blur_amount);
	cfg->spotlight.zoom_amount =
		read_number(o, kSpotlightZoomAmount, cfg->spotlight.zoom_amount);
	cfg->spotlight.in_ms = read_number(o, kSpotlightInMs, cfg->spotlight.in_ms);
	cfg->spotlight.out_ms = read_number(o, kSpotlightOutMs, cfg->spotlight.out_ms);
	cfg->spotlight.show_cursor = read_bool(o, kSpotlightShowCursor, cfg->spotlight.show_cursor);

	/* Magnifier */
	cfg->magnifier.enabled = read_bool(o, kMagnifierEnabled, cfg->magnifier.enabled);
	cfg->magnifier.shape = read_int(o, kMagnifierShape, cfg->magnifier.shape);
	cfg->magnifier.radius = read_number(o, kMagnifierRadius, cfg->magnifier.radius);
	cfg->magnifier.corner_radius =
		read_number(o, kMagnifierCornerRadius, cfg->magnifier.corner_radius);
	cfg->magnifier.zoom_amount = read_number(o, kMagnifierZoom, cfg->magnifier.zoom_amount);
	cfg->magnifier.border_enabled =
		read_bool(o, kMagnifierBorderEnabled, cfg->magnifier.border_enabled);
	cfg->magnifier.border_color =
		static_cast<uint32_t>(read_number(o, kMagnifierBorderColor, cfg->magnifier.border_color));
	cfg->magnifier.border_width =
		read_number(o, kMagnifierBorderWidth, cfg->magnifier.border_width);
	cfg->magnifier.shadow_enabled =
		read_bool(o, kMagnifierShadowEnabled, cfg->magnifier.shadow_enabled);
	cfg->magnifier.shadow_color =
		static_cast<uint32_t>(read_number(o, kMagnifierShadowColor, cfg->magnifier.shadow_color));
	cfg->magnifier.shadow_alpha =
		read_number(o, kMagnifierShadowAlpha, cfg->magnifier.shadow_alpha);
	cfg->magnifier.shadow_blur =
		read_number(o, kMagnifierShadowBlur, cfg->magnifier.shadow_blur);
	cfg->magnifier.shadow_offset_x =
		read_number(o, kMagnifierShadowOffsetX, cfg->magnifier.shadow_offset_x);
	cfg->magnifier.shadow_offset_y =
		read_number(o, kMagnifierShadowOffsetY, cfg->magnifier.shadow_offset_y);
	cfg->magnifier.follow_cursor =
		read_bool(o, kMagnifierFollowCursor, cfg->magnifier.follow_cursor);
	cfg->magnifier.center_x = read_number(o, kMagnifierCenterX, cfg->magnifier.center_x);
	cfg->magnifier.center_y = read_number(o, kMagnifierCenterY, cfg->magnifier.center_y);
	cfg->magnifier.in_ms = read_number(o, kMagnifierInMs, cfg->magnifier.in_ms);
	cfg->magnifier.out_ms = read_number(o, kMagnifierOutMs, cfg->magnifier.out_ms);
	cfg->magnifier.show_cursor = read_bool(o, kMagnifierShowCursor, cfg->magnifier.show_cursor);

	/* Keys */
	cfg->keys.enabled = read_bool(o, kKeysEnabled, cfg->keys.enabled);
	cfg->keys.modifiers_only = read_bool(o, kKeysModifiersOnly, cfg->keys.modifiers_only);
	cfg->keys.show_special = read_bool(o, kKeysShowSpecial, cfg->keys.show_special);
	cfg->keys.show_shortcuts =
		read_bool(o, kKeysShowShortcuts, cfg->keys.show_shortcuts);
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
	cfg->highlight.zoom_delay_ms = read_number(o, kHighlightZoomDelayMs, cfg->highlight.zoom_delay_ms);
	cfg->highlight.zoom_max = read_number(o, kHighlightZoomMax, cfg->highlight.zoom_max);
	cfg->highlight.zoom_in_ms = read_number(o, kHighlightZoomInMs, cfg->highlight.zoom_in_ms);
	cfg->highlight.zoom_hold_ms = read_number(o, kHighlightZoomHoldMs, cfg->highlight.zoom_hold_ms);
	cfg->highlight.zoom_out_ms = read_number(o, kHighlightZoomOutMs, cfg->highlight.zoom_out_ms);

	cfg->monitor_manual = read_bool(o, kMonitorManual, cfg->monitor_manual);
	cfg->monitor_x = static_cast<int>(read_int(o, kMonitorX, cfg->monitor_x));
	cfg->monitor_y = static_cast<int>(read_int(o, kMonitorY, cfg->monitor_y));

	sanitize_config(cfg);
	return true;
}

} // namespace topscreen::core
