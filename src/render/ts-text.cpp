#include "ts-text.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

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
const char *icon_for_label(std::string_view label)
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
	/* The cached geometry points into the old atlas: drop it before the
	 * atlas is replaced. */
	destroy_geometry();
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
	fallback_glyph_ = atlas_.glyph('?');
	layout_glyphs_.clear();
	layout_glyphs_.reserve(static_cast<size_t>(kMaxLabels * (kMaxLabelBytes + 1)));
	layout_valid_ = false;
	cached_count_ = 0;
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
	if (texture_ || white_ || effect_ || glyph_vb_) {
		obs_enter_graphics();
		destroy_geometry();
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
	fallback_glyph_ = nullptr;
	layout_glyphs_.clear();
	layout_valid_ = false;
	cached_count_ = 0;
	run_count_ = 0;
}

bool TextRenderer::build_layout(const core::KeyRender *items, int count)
{
	if (count > kMaxLabels)
		count = kMaxLabels;

	/* Resolve icons and the 63-byte limit once, then compare with the cached
	 * labels. Progress/TTL do not change the glyphs. */
	std::array<std::string_view, kMaxLabels> views{};
	for (int i = 0; i < count; ++i) {
		std::string_view view = items[i].label.view();
		if (view.size() > kMaxLabelBytes)
			view = view.substr(0, kMaxLabelBytes);
		if (const char *icon = icon_for_label(view)) {
			int cp = 0;
			const size_t n = utf8_next(icon, 3, &cp);
			if (atlas_.glyph(cp))
				view = std::string_view(icon, n);
		}
		views[i] = view;
	}

	bool changed = !layout_valid_ || count != cached_count_;
	if (!changed) {
		for (int i = 0; i < count; ++i) {
			if (views[i] != cached_labels_[i].view()) {
				changed = true;
				break;
			}
		}
	}
	if (!changed)
		return false;

	for (int i = 0; i < count; ++i)
		cached_labels_[i].assign(views[i]);
	cached_count_ = count;

	layout_glyphs_.clear();
	run_count_ = 0;
	for (int i = 0; i < count; ++i) {
		Run &run = runs_[run_count_++];
		run.first = static_cast<int>(layout_glyphs_.size());
		run.width = 0.0f;

		const std::string_view text = views[i];
		for (size_t pos = 0; pos < text.size();) {
			int cp = 0;
			pos += utf8_next(text.data() + pos, text.size() - pos, &cp);
			const core::Glyph *g = atlas_.glyph(cp);
			if (!g)
				g = fallback_glyph_;
			if (!g)
				continue;
			layout_glyphs_.push_back(g);
			run.width += static_cast<float>(g->advance);
		}
		run.count = static_cast<int>(layout_glyphs_.size()) - run.first;
	}

	layout_valid_ = true;
	return true;
}

void TextRenderer::destroy_geometry()
{
	if (glyph_vb_) {
		gs_vertexbuffer_destroy(glyph_vb_);
		glyph_vb_ = nullptr;
	}
	glyph_vertex_count_ = 0;
}

void TextRenderer::rebuild_geometry()
{
	/* Called with the graphics context active (inside draw/reload). */
	destroy_geometry();
	if (layout_glyphs_.empty() || atlas_.width <= 0 || atlas_.height <= 0)
		return;

	uint32_t vertices = 0;
	for (const core::Glyph *g : layout_glyphs_) {
		if (g->w > 0 && g->h > 0)
			vertices += 6;
	}
	if (vertices == 0)
		return;

	const float atlas_w = static_cast<float>(atlas_.width);
	const float atlas_h = static_cast<float>(atlas_.height);

	gs_render_start(true);
	float pen = 0.0f;
	for (int r = 0; r < run_count_; ++r) {
		const Run &run = runs_[r];
		for (int i = 0; i < run.count; ++i) {
			const core::Glyph *g = layout_glyphs_[run.first + i];
			if (g->w > 0 && g->h > 0) {
				const float x0 = pen + static_cast<float>(g->xoff);
				const float y0 = static_cast<float>(g->yoff);
				const float x1 = x0 + static_cast<float>(g->w);
				const float y1 = y0 + static_cast<float>(g->h);
				const float u0 = static_cast<float>(g->x) / atlas_w;
				const float v0 = static_cast<float>(g->y) / atlas_h;
				const float u1 = static_cast<float>(g->x + g->w) / atlas_w;
				const float v1 = static_cast<float>(g->y + g->h) / atlas_h;

				gs_vertex2f(x0, y0);
				gs_texcoord(u0, v0, 0);
				gs_vertex2f(x1, y0);
				gs_texcoord(u1, v0, 0);
				gs_vertex2f(x0, y1);
				gs_texcoord(u0, v1, 0);
				gs_vertex2f(x1, y0);
				gs_texcoord(u1, v0, 0);
				gs_vertex2f(x1, y1);
				gs_texcoord(u1, v1, 0);
				gs_vertex2f(x0, y1);
				gs_texcoord(u0, v1, 0);
			}
			pen += static_cast<float>(g->advance);
		}
		/* Matches the previous gap of 14.0f * scale (same units as the
		 * advances, which are scaled by the draw matrix). */
		pen += kGapUnits;
	}

	glyph_vb_ = gs_render_save();
	if (glyph_vb_)
		glyph_vertex_count_ = vertices;
}

void TextRenderer::draw_geometry(float x, float baseline, float scale, uint32_t color)
{
	if (!glyph_vb_ || glyph_vertex_count_ == 0)
		return;

	if (p_color_)
		gs_effect_set_color(p_color_, color);
	gs_matrix_push();
	gs_matrix_translate3f(x, baseline, 0.0f);
	gs_matrix_scale3f(scale, scale, 1.0f);
	gs_draw(GS_TRIS, 0, glyph_vertex_count_);
	gs_matrix_pop();
}

void TextRenderer::draw_geometry_fallback(float x, float baseline, float scale, uint32_t color)
{
	if (p_color_)
		gs_effect_set_color(p_color_, color);
	gs_matrix_push();
	gs_matrix_translate3f(x, baseline, 0.0f);

	float pen = 0.0f;
	for (int r = 0; r < run_count_; ++r) {
		const Run &run = runs_[r];
		for (int i = 0; i < run.count; ++i) {
			const core::Glyph *g = layout_glyphs_[run.first + i];
			if (g->w > 0 && g->h > 0) {
				gs_matrix_push();
				gs_matrix_translate3f(pen + static_cast<float>(g->xoff) * scale,
						      static_cast<float>(g->yoff) * scale, 0.0f);
				gs_matrix_scale3f(scale, scale, 1.0f);
				gs_draw_sprite_subregion(texture_, 0, static_cast<uint32_t>(g->x),
							 static_cast<uint32_t>(g->y),
							 static_cast<uint32_t>(g->w),
							 static_cast<uint32_t>(g->h));
				gs_matrix_pop();
			}
			pen += static_cast<float>(g->advance) * scale;
		}
		pen += kGapUnits * scale;
	}

	gs_matrix_pop();
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
	/* A failed texture creation frees the old texture: do not draw the
	 * cached layout against a null texture. */
	if (!loaded())
		return;

	/* Decode/look up the glyphs only when the text or the font changed. */
	if (build_layout(items, count))
		rebuild_geometry();

	const float base = static_cast<float>(target_h) * 0.030f;
	const float scale = std::max(1.0f, base * static_cast<float>(cfg.size) / 32.0f);
	const float line_h = static_cast<float>(atlas_.line_height) * scale;

	float total_units = 0.0f;
	for (int i = 0; i < run_count_; ++i) {
		total_units += runs_[i].width;
		if (i + 1 < run_count_)
			total_units += kGapUnits;
	}
	const float total = total_units * scale;

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

	/* All glyphs share one vertex buffer: bind it once and draw it per
	 * shadow offset/main pass. */
	gs_load_indexbuffer(nullptr);
	if (glyph_vb_)
		gs_load_vertexbuffer(glyph_vb_);

	const float o = std::max(1.0f, 2.0f * scale);
	const float offs[4][2] = {{-o, 0.0f}, {o, 0.0f}, {0.0f, -o}, {0.0f, o}};
	if (glyph_vb_) {
		if (cfg.shadow) {
			for (const auto &of : offs)
				draw_geometry(x0 + of[0], baseline + of[1], scale, 0xFF000000);
		}
		draw_geometry(x0, baseline, scale, core::abgr_to_argb(cfg.color));
	} else {
		if (cfg.shadow) {
			for (const auto &of : offs)
				draw_geometry_fallback(x0 + of[0], baseline + of[1], scale,
						       0xFF000000);
		}
		draw_geometry_fallback(x0, baseline, scale, core::abgr_to_argb(cfg.color));
	}

	gs_technique_end_pass(tech_draw_);
	gs_technique_end(tech_draw_);
	gs_blend_state_pop();
}

} // namespace topscreen
