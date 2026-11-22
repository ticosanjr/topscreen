#include "ts-settings.hpp"

#include "core/cursor_catalog.hpp"
#include "core/config_fields.hpp"
#include "core/keys.hpp"
#include "ts-wallpapers.hpp"

#include <algorithm>

using namespace topscreen::core;
using namespace topscreen::core::keys;

namespace topscreen {

#define TS_DEF_BOOL(k, m) obs_data_set_default_bool(s, k, def.m)
#define TS_DEF_INT(k, m) obs_data_set_default_int(s, k, def.m)
#define TS_DEF_DOUBLE(k, m) obs_data_set_default_double(s, k, def.m)
#define TS_DEF_COLOR(k, m) obs_data_set_default_int(s, k, static_cast<long long>(def.m))
#define TS_DEF_STRING(k, m) obs_data_set_default_string(s, k, def.m.c_str())

void config_defaults(obs_data_t *s)
{
	const Config def;
#define X(kind, key, member) TS_DEF_##kind(key, member);
	TS_CONFIG_FIELDS(X)
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

/*
 * OBS's plain color picker rewrites the stored value with alpha 255, so a
 * translucent color edited in Properties would suddenly become opaque. The
 * highlighter used to carry transparency in the color byte; fold it into the
 * explicit opacity once and keep the color RGB-only from now on. (The key band
 * has no separate opacity, so it uses an alpha color picker instead.) The
 * marker lives in the same obs_data as the setting (the source or the profile
 * slot), so older scenes migrate exactly once.
 */
constexpr const char *kColorAlphaMigrated = "color_alpha_rgb_v1";

void migrate_translucent_colors(obs_data_t *s, Config *c)
{
	if (!s || !c || obs_data_has_user_value(s, kColorAlphaMigrated))
		return;

	bool changed = false;
	auto fold = [&](uint32_t *color, double *opacity) {
		const uint32_t alpha = (*color >> 24) & 0xFFu;
		if (alpha == 0xFFu)
			return;
		if (opacity)
			*opacity = std::clamp(*opacity * (static_cast<double>(alpha) / 255.0),
					      0.0, 1.0);
		*color |= 0xFF000000u;
		changed = true;
	};

	fold(&c->highlight.color, &c->highlight.opacity);

	if (!changed)
		return;

	obs_data_set_int(s, kHighlightColor, static_cast<long long>(c->highlight.color));
	obs_data_set_double(s, kHighlightOpacity, c->highlight.opacity);
	obs_data_set_bool(s, kColorAlphaMigrated, true);
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
	TS_CONFIG_FIELDS(X)
#undef X

	/* Defaults are not enough to tell a legacy source from a sparse profile
	 * slot: only a value the user actually wrote means "migrate me". When
	 * neither key is present, keep the fallback (the profile seed). */
	const bool has_type = obs_data_has_user_value(s, kBgType);
	const bool has_legacy = obs_data_has_user_value(s, kBgMode);
	const long long legacy_mode = has_legacy
		? read_int(s, kBgMode, legacy_bg_mode_value(d.bg.mode))
		: legacy_bg_mode_value(d.bg.mode);
	int mode = static_cast<int>(d.bg.mode);
	if (has_type) {
		mode = static_cast<int>(read_int(s, kBgType, d.bg.mode));
	} else if (has_legacy) {
		const bool has_wallpaper = !c->bg.image.empty() || !c->bg.image_file.empty();
		mode = bg_mode_from_legacy(static_cast<int>(legacy_mode), has_wallpaper);
	}
	/* An empty/sparse slot with no background keys keeps the fallback seed. */
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

	if (!has_type && has_legacy && !obs_data_has_user_value(s, kBgGradientAngle))
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
	migrate_translucent_colors(s, c);
}

void config_store(obs_data_t *s, const Config &c)
{
#define X(kind, key, member) TS_STORE_##kind(key, member);
	TS_CONFIG_FIELDS(X)
#undef X
	obs_data_set_int(s, kBgType, c.bg.mode);
	obs_data_set_int(s, kBgMode, legacy_bg_mode_value(c.bg.mode));
}

void config_store_sparse(obs_data_t *s, const Config &c, const Config &base)
{
#define X(kind, key, member) \
	if (c.member != base.member) \
		TS_STORE_##kind(key, member);
	TS_CONFIG_FIELDS(X)
#undef X
	if (c.bg.mode != base.bg.mode) {
		obs_data_set_int(s, kBgType, c.bg.mode);
		obs_data_set_int(s, kBgMode, legacy_bg_mode_value(c.bg.mode));
	}
}

} // namespace topscreen
