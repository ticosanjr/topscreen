#include "ts-text.hpp"

#include <algorithm>
#include <cctype>

#include <graphics/vec2.h>
#include <obs-module.h>

#include "core/color.hpp"
#include "core/file.hpp"

namespace topscreen {

namespace {

bool read_module_bytes(const char *module_file, std::vector<uint8_t> *out)
{
	char *path = obs_module_file(module_file);
	if (!path)
		return false;

	const bool ok = core::read_file_bytes(path, out);
	bfree(path);
	return ok;
}

/* Special keys are shown as icons (one symbol each, UTF-8). */
const char *icon_for_label(const std::string &label)
{
	static const char *const kLabels[] = {
		"Space", "Backspace", "Enter", "Tab", "Left", "Up",   "Right",
		"Down",  "Shift",     "Ctrl",  "Alt", "Win",  "Del",  "Delete",
	};
	/* U+2423, U+232B, U+23CE, U+21E5, U+2190, U+2191, U+2192,
	 * U+2193, U+21E7, U+2303, U+2325, U+229E, U+2326, U+2326.
	 * ("Esc" stays as text: the bundled font has no escape symbol.) */
	static const char *const kIcons[] = {
		"\xE2\x90\xA3", "\xE2\x8C\xAB", "\xE2\x8E\x8E", "\xE2\x87\xA5",
		"\xE2\x86\x90", "\xE2\x86\x91", "\xE2\x86\x92", "\xE2\x86\x93", "\xE2\x87\xA7",
		"\xE2\x8C\x83", "\xE2\x8C\xA5", "\xE2\x8A\x9E", "\xE2\x8C\xA6", "\xE2\x8C\xA6",
	};
	for (size_t i = 0; i < sizeof(kLabels) / sizeof(kLabels[0]); ++i) {
		if (label == kLabels[i])
			return kIcons[i];
	}
	return nullptr;
}

/* Decodes one UTF-8 codepoint; always consumes at least one byte. */
size_t utf8_next(const char *s, size_t n, int *code)
{
	const auto b = static_cast<unsigned char>(s[0]);
	if (b < 0x80) {
		*code = b;
		return 1;
	}
	if ((b & 0xE0) == 0xC0 && n >= 2) {
		*code = ((b & 0x1F) << 6) | (static_cast<unsigned char>(s[1]) & 0x3F);
		return 2;
	}
	if ((b & 0xF0) == 0xE0 && n >= 3) {
		*code = ((b & 0x0F) << 12) | ((static_cast<unsigned char>(s[1]) & 0x3F) << 6) |
			(static_cast<unsigned char>(s[2]) & 0x3F);
		return 3;
	}
	*code = '?';
	return 1;
}

} // namespace

TextRenderer::~TextRenderer()
{
	release();
}

bool TextRenderer::reload_font(const std::string &module_file)
{
	if (!module_file.empty() && module_file == font_path_ && texture_)
		return true;

	std::vector<uint8_t> ttf;
	if (!read_module_bytes(module_file.c_str(), &ttf)) {
		blog(LOG_WARNING, "[TopScreen] font not found: %s", module_file.c_str());
		return false;
	}

	core::FontAtlasOptions opt;
	opt.pixel_height = 32;

	core::FontAtlas atlas;
	std::string err;
	if (!core::font_atlas_build(ttf.data(), ttf.size(), opt, &atlas, &err)) {
		blog(LOG_WARNING, "[TopScreen] failed to build the font atlas: %s", err.c_str());
		return false;
	}

	obs_enter_graphics();
	if (texture_) {
		gs_texture_destroy(texture_);
		texture_ = nullptr;
	}
	const uint8_t *data[1] = {atlas.pixels.data()};
	texture_ = gs_texture_create(static_cast<uint32_t>(atlas.width), static_cast<uint32_t>(atlas.height),
				     GS_RGBA, 1, data, 0);
	obs_leave_graphics();

	if (!texture_) {
		blog(LOG_WARNING, "[TopScreen] failed to create the font texture");
		return false;
	}

	atlas_ = std::move(atlas);
	font_path_ = module_file;
	return true;
}

bool TextRenderer::load(const char *ttf_module_file, const char *effect_module_file)
{
	release();

	if (!ttf_module_file || !*ttf_module_file)
		return false;
	if (!reload_font(ttf_module_file))
		return false;

	char *fx_path = obs_module_file(effect_module_file);
	if (!fx_path) {
		blog(LOG_WARNING, "[TopScreen] text effect not found: %s", effect_module_file);
		return false;
	}

	obs_enter_graphics();
	effect_ = gs_effect_create_from_file(fx_path, nullptr);
	if (!white_) {
		const uint8_t pixel[4] = {255, 255, 255, 255};
		const uint8_t *data[1] = {pixel};
		white_ = gs_texture_create(1, 1, GS_RGBA, 1, data, 0);
	}
	obs_leave_graphics();

	bfree(fx_path);

	if (!effect_) {
		blog(LOG_WARNING, "[TopScreen] failed to create the text effect");
		release();
		return false;
	}

	tech_draw_ = gs_effect_get_technique(effect_, "Draw");
	tech_band_ = gs_effect_get_technique(effect_, "Band");
	p_image_ = gs_effect_get_param_by_name(effect_, "image");
	p_color_ = gs_effect_get_param_by_name(effect_, "color");
	p_band_size_ = gs_effect_get_param_by_name(effect_, "band_size");
	p_band_radius_ = gs_effect_get_param_by_name(effect_, "band_radius");
	p_band_color_ = gs_effect_get_param_by_name(effect_, "band_color");
	return true;
}

void TextRenderer::release()
{
	if (texture_ || white_ || effect_) {
		obs_enter_graphics();
		if (texture_) {
			gs_texture_destroy(texture_);
			texture_ = nullptr;
		}
		if (white_) {
			gs_texture_destroy(white_);
			white_ = nullptr;
		}
		if (effect_) {
			gs_effect_destroy(effect_);
			effect_ = nullptr;
		}
		obs_leave_graphics();
	}
	tech_draw_ = tech_band_ = nullptr;
	p_image_ = p_color_ = nullptr;
	p_band_size_ = p_band_radius_ = p_band_color_ = nullptr;
	font_path_.clear();
}

float TextRenderer::measure(const std::string &text, float scale) const
{
	float w = 0.0f;
	for (size_t i = 0; i < text.size();) {
		int cp = 0;
		i += utf8_next(text.data() + i, text.size() - i, &cp);
		const core::Glyph *g = atlas_.glyph(cp);
		if (!g)
			g = atlas_.glyph('?');
		if (g)
			w += static_cast<float>(g->advance) * scale;
	}
	return w;
}

void TextRenderer::draw_string(const std::string &text, float x, float baseline, float scale)
{
	float pen = x;
	for (size_t i = 0; i < text.size();) {
		int cp = 0;
		i += utf8_next(text.data() + i, text.size() - i, &cp);
		const core::Glyph *g = atlas_.glyph(cp);
		if (!g)
			g = atlas_.glyph('?');
		if (!g)
			continue;

		if (g->w > 0 && g->h > 0) {
			gs_matrix_push();
			gs_matrix_translate3f(pen + static_cast<float>(g->xoff) * scale,
					      baseline + static_cast<float>(g->yoff) * scale, 0.0f);
			gs_matrix_scale3f(scale, scale, 1.0f);
			gs_draw_sprite_subregion(texture_, 0, static_cast<uint32_t>(g->x),
						 static_cast<uint32_t>(g->y), static_cast<uint32_t>(g->w),
						 static_cast<uint32_t>(g->h));
			gs_matrix_pop();
		}

		pen += static_cast<float>(g->advance) * scale;
	}
}

void TextRenderer::draw_labels(const std::vector<std::string> &labels, const std::vector<float> &widths,
				 float x0, float baseline, float scale, float gap)
{
	float x = x0;
	for (size_t i = 0; i < labels.size(); ++i) {
		draw_string(labels[i], x, baseline, scale);
		x += (i < widths.size() ? widths[i] : measure(labels[i], scale)) + gap;
	}
}

void TextRenderer::draw_band(float x, float y, float w, float h, float radius, uint32_t color)
{
	if (!tech_band_ || !white_)
		return;

	struct vec2 size;
	vec2_set(&size, w, h);
	if (p_band_size_)
		gs_effect_set_vec2(p_band_size_, &size);
	if (p_band_radius_)
		gs_effect_set_float(p_band_radius_, radius);
	if (p_band_color_)
		gs_effect_set_color(p_band_color_, core::abgr_to_argb(color));

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
	gs_technique_begin(tech_band_);
	gs_technique_begin_pass(tech_band_, 0);
	gs_matrix_push();
	gs_matrix_translate3f(x, y, 0.0f);
	gs_matrix_scale3f(w, h, 1.0f);
	gs_draw_sprite_subregion(white_, 0, 0, 0, 1, 1);
	gs_matrix_pop();
	gs_technique_end_pass(tech_band_);
	gs_technique_end(tech_band_);
	gs_blend_state_pop();
}

void TextRenderer::draw(const core::KeyRender *items, int count, const core::KeyOverlayConfig &cfg,
			uint32_t target_w, uint32_t target_h)
{
	if (!loaded() || !items || count <= 0 || target_w == 0 || target_h == 0)
		return;

	if (cfg.font != font_path_)
		reload_font(cfg.font);

	std::vector<std::string> labels;
	labels.reserve(static_cast<size_t>(count));
	std::vector<float> widths;
	widths.reserve(static_cast<size_t>(count));
	for (int i = 0; i < count; ++i) {
		/* Shows what was typed (no forced case); special keys become icons.
		 * Falls back to the text label when the font lacks the symbol. */
		std::string s = items[i].label;
		if (const char *icon = icon_for_label(items[i].label)) {
			int cp = 0;
			utf8_next(icon, 3, &cp);
			if (atlas_.glyph(cp))
				s = icon;
		}
		labels.push_back(s);
	}

	const float base = static_cast<float>(target_h) * 0.030f;
	const float scale = std::max(1.0f, base * static_cast<float>(cfg.size) / 32.0f);
	const float gap = 14.0f * scale;
	const float line_h = static_cast<float>(atlas_.line_height) * scale;

	float total = 0.0f;
	for (const std::string &s : labels) {
		const float w = measure(s, scale);
		widths.push_back(w);
		total += w;
	}
	if (labels.size() > 1)
		total += gap * static_cast<float>(labels.size() - 1);

	const float cx = static_cast<float>(cfg.pos_x) * static_cast<float>(target_w);
	const float baseline = static_cast<float>(cfg.pos_y) * static_cast<float>(target_h);
	const float x0 = cx - total * 0.5f;

	if (cfg.band) {
		const float padx = 20.0f * scale;
		const float pady = 10.0f * scale;
		const float bx = x0 - padx;
		const float by = baseline - static_cast<float>(atlas_.ascent) * scale - pady;
		draw_band(bx, by, total + padx * 2.0f, line_h + pady * 2.0f, 12.0f * scale, cfg.band_color);
	}

	if (!tech_draw_)
		return;

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
	gs_technique_begin(tech_draw_);
	gs_technique_begin_pass(tech_draw_, 0);
	if (p_image_)
		gs_effect_set_texture(p_image_, texture_);

	if (cfg.shadow) {
		if (p_color_)
			gs_effect_set_color(p_color_, 0xFF000000);
		const float o = std::max(1.0f, 2.0f * scale);
		const float offs[4][2] = {{-o, 0.0f}, {o, 0.0f}, {0.0f, -o}, {0.0f, o}};
		for (const auto &of : offs)
			draw_labels(labels, widths, x0 + of[0], baseline + of[1], scale, gap);
	}

	if (p_color_)
		gs_effect_set_color(p_color_, core::abgr_to_argb(cfg.color));
	draw_labels(labels, widths, x0, baseline, scale, gap);

	gs_technique_end_pass(tech_draw_);
	gs_technique_end(tech_draw_);
	gs_blend_state_pop();
}

} // namespace topscreen
