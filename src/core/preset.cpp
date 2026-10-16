#include "preset.hpp"

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
			*error = "preset: raiz nao e um objeto";
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
			*error = "preset: falta o objeto 'settings'";
		return false;
	}

	out->settings_ = *settings;
	return true;
}

bool Preset::apply(Config *cfg, std::string *error) const
{
	if (!settings_.is_object()) {
		if (error)
			*error = "preset: 'settings' nao e um objeto";
		return false;
	}

	const auto &o = *settings_.object;
	using namespace keys;

	cfg->effects_enabled = read_bool(o, kEffectsEnabled, cfg->effects_enabled);

	/* Fundo */
	cfg->bg.enabled = read_bool(o, kBgEnabled, cfg->bg.enabled);
	cfg->bg.mode = read_int(o, kBgMode, cfg->bg.mode);
	cfg->bg.color_a = static_cast<uint32_t>(read_number(o, kBgColorA, cfg->bg.color_a));
	cfg->bg.color_b = static_cast<uint32_t>(read_number(o, kBgColorB, cfg->bg.color_b));
	cfg->bg.padding = read_number(o, kBgPadding, cfg->bg.padding);
	cfg->bg.corner_radius = read_number(o, kBgCornerRadius, cfg->bg.corner_radius);
	cfg->bg.shadow = read_bool(o, kBgShadow, cfg->bg.shadow);
	cfg->bg.shadow_alpha = read_number(o, kBgShadowAlpha, cfg->bg.shadow_alpha);
	cfg->bg.shadow_blur = read_number(o, kBgShadowBlur, cfg->bg.shadow_blur);

	/* Cursor */
	cfg->cursor.enabled = read_bool(o, kCursorEnabled, cfg->cursor.enabled);
	cfg->cursor.style = read_int(o, kCursorStyle, cfg->cursor.style);
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

	/* Clique esquerdo */
	cfg->click_left.effect = read_int(o, kClickLeftEffect, cfg->click_left.effect);
	cfg->click_left.size = read_number(o, kClickLeftSize, cfg->click_left.size);
	cfg->click_left.color = static_cast<uint32_t>(read_number(o, kClickLeftColor, cfg->click_left.color));
	cfg->click_left.duration_ms = read_number(o, kClickLeftDurationMs, cfg->click_left.duration_ms);
	cfg->click_left.easing = read_int(o, kClickLeftEasing, cfg->click_left.easing);

	/* Clique direito */
	cfg->click_right.effect = read_int(o, kClickRightEffect, cfg->click_right.effect);
	cfg->click_right.size = read_number(o, kClickRightSize, cfg->click_right.size);
	cfg->click_right.color =
		static_cast<uint32_t>(read_number(o, kClickRightColor, cfg->click_right.color));
	cfg->click_right.duration_ms = read_number(o, kClickRightDurationMs, cfg->click_right.duration_ms);
	cfg->click_right.easing = read_int(o, kClickRightEasing, cfg->click_right.easing);

	/* Som de clique */
	cfg->click_sound.enabled = read_bool(o, kClickSoundEnabled, cfg->click_sound.enabled);
	cfg->click_sound.file = read_string(o, kClickSoundFile, cfg->click_sound.file);
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

	/* Teclas */
	cfg->keys.enabled = read_bool(o, kKeysEnabled, cfg->keys.enabled);
	cfg->keys.modifiers_only = read_bool(o, kKeysModifiersOnly, cfg->keys.modifiers_only);
	cfg->keys.ttl_ms = read_number(o, kKeysTtlMs, cfg->keys.ttl_ms);
	cfg->keys.max_keys = read_int(o, kKeysMax, cfg->keys.max_keys);

	/* Marca-texto */
	cfg->highlight.enabled = read_bool(o, kHighlightEnabled, cfg->highlight.enabled);
	cfg->highlight.hold_ms = read_number(o, kHighlightHoldMs, cfg->highlight.hold_ms);
	cfg->highlight.fade_ms = read_number(o, kHighlightFadeMs, cfg->highlight.fade_ms);
	cfg->highlight.color = static_cast<uint32_t>(read_number(o, kHighlightColor, cfg->highlight.color));

	return true;
}

} // namespace topscreen::core
