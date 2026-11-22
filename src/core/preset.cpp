#include "preset.hpp"

#include "cursor_catalog.hpp"
#include "config_fields.hpp"
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

	/* Simple fields come from the same table as the settings loader, so a new
	 * Config field cannot be forgotten here. Missing keys keep the value that
	 * already exists in `cfg` (read_* uses it as the fallback). */
#define TS_PRESET_READ_BOOL(_o, _k, _m) read_bool(_o, _k, _m)
#define TS_PRESET_READ_INT(_o, _k, _m) read_int(_o, _k, _m)
#define TS_PRESET_READ_DOUBLE(_o, _k, _m) read_number(_o, _k, _m)
#define TS_PRESET_READ_COLOR(_o, _k, _m) static_cast<uint32_t>(read_number(_o, _k, _m))
#define TS_PRESET_READ_STRING(_o, _k, _m) read_string(_o, _k, _m)
#define X(kind, key, member) cfg->member = TS_PRESET_READ_##kind(o, key, cfg->member);
	TS_CONFIG_FIELDS(X)
#undef X
#undef TS_PRESET_READ_BOOL
#undef TS_PRESET_READ_INT
#undef TS_PRESET_READ_DOUBLE
#undef TS_PRESET_READ_COLOR
#undef TS_PRESET_READ_STRING

	/* Background type: `bg_type` wins; otherwise migrate the legacy `bg_mode`. */
	if (lookup(o, kBgType)) {
		cfg->bg.mode = read_int(o, kBgType, cfg->bg.mode);
	} else {
		const int legacy = read_int(o, kBgMode, cfg->bg.mode);
		const bool has_wallpaper = !cfg->bg.image.empty() || !cfg->bg.image_file.empty();
		cfg->bg.mode = bg_mode_from_legacy(legacy, has_wallpaper);
		if (!lookup(o, kBgGradientAngle))
			cfg->bg.gradient_angle =
				bg_gradient_angle_from_legacy(legacy, cfg->bg.gradient_angle);
	}
	if (cfg->bg.mode < 0 || cfg->bg.mode > static_cast<int>(BgMode::BlurredScreen))
		cfg->bg.mode = static_cast<int>(BgMode::Gradient);
	if (cfg->bg.mode == static_cast<int>(BgMode::Wallpaper)) {
		if (cfg->bg.image == "__transparent__")
			cfg->bg.mode = static_cast<int>(BgMode::Transparent);
		else if (cfg->bg.image == "__local__")
			cfg->bg.mode = static_cast<int>(BgMode::LocalImage);
		else if (cfg->bg.image.empty() && !cfg->bg.image_file.empty())
			cfg->bg.mode = static_cast<int>(BgMode::LocalImage);
	}

	/* Retired cursor IDs are clamped here, before the final sanitize. */
	cfg->cursor.style = cursor_style_clamp(cfg->cursor.style);

	/* Legacy single-file presets apply to both buttons. */
	if (o.find(kClickSoundFileLeft) == o.end() && o.find(kClickSoundFileRight) == o.end() &&
	    o.find(kClickSoundFile) != o.end()) {
		const std::string old = read_string(o, kClickSoundFile, "");
		if (!old.empty()) {
			cfg->click_sound.file_left = old;
			cfg->click_sound.file_right = old;
		}
	}
	/* Legacy per-side booleans: false meant muted before the Mute list items. */
	if (!read_bool(o, kClickSoundLeft, true))
		cfg->click_sound.file_left.clear();
	if (!read_bool(o, kClickSoundRight, true))
		cfg->click_sound.file_right.clear();
	cfg->click_sound.left = !cfg->click_sound.file_left.empty();
	cfg->click_sound.right = !cfg->click_sound.file_right.empty();

	/* Presets written before the explicit mode only had blur_enabled. */
	if (!lookup(o, kSpotlightOutsideMode) && cfg->spotlight.blur_enabled)
		cfg->spotlight.outside_mode = 1;

	sanitize_config(cfg);
	return true;
}

} // namespace topscreen::core
