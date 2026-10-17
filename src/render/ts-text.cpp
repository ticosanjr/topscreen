#include "ts-text.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>

#include <graphics/vec2.h>
#include <obs-module.h>

#include "core/color.hpp"

namespace topscreen {

namespace {

bool read_module_bytes(const char *module_file, std::vector<uint8_t> *out)
{
	char *path = obs_module_file(module_file);
	if (!path)
		return false;

	std::ifstream in(path, std::ios::binary);
	bfree(path);
	if (!in)
		return false;

	*out = std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	return !out->empty();
}

std::string upper_copy(const std::string &s)
{
	std::string out = s;
	for (char &c : out)
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	return out;
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
		blog(LOG_WARNING, "[TopScreen] fonte nao encontrada: %s", module_file.c_str());
		return false;
	}

	core::FontAtlasOptions opt;
	opt.pixel_height = 32;

	core::FontAtlas atlas;
	std::string err;
	if (!core::font_atlas_build(ttf.data(), ttf.size(), opt, &atlas, &err)) {
		blog(LOG_WARNING, "[TopScreen] falha ao gerar o atlas da fonte: %s", err.c_str());
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
		blog(LOG_WARNING, "[TopScreen] falha ao criar a textura da fonte");
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
		blog(LOG_WARNING, "[TopScreen] efeito de texto nao encontrado: %s", effect_module_file);
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
		blog(LOG_WARNING, "[TopScreen] falha ao criar o efeito de texto");
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
	for (char c : text) {
		const core::Glyph *g = atlas_.glyph(static_cast<unsigned char>(c));
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
	for (char c : text) {
		const core::Glyph *g = atlas_.glyph(static_cast<unsigned char>(c));
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

void TextRenderer::draw_labels(const std::vector<std::string> &labels, float x0, float baseline, float scale,
			       float gap)
{
	float x = x0;
	for (const std::string &s : labels) {
		draw_string(s, x, baseline, scale);
		x += measure(s, scale) + gap;
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
	for (int i = 0; i < count; ++i)
		labels.push_back(upper_copy(items[i].label));

	const float base = static_cast<float>(target_h) * 0.030f;
	const float scale = std::max(1.0f, base * static_cast<float>(cfg.size) / 32.0f);
	const float gap = 14.0f * scale;
	const float line_h = static_cast<float>(atlas_.line_height) * scale;

	float total = 0.0f;
	for (const std::string &s : labels)
		total += measure(s, scale);
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
			draw_labels(labels, x0 + of[0], baseline + of[1], scale, gap);
	}

	if (p_color_)
		gs_effect_set_color(p_color_, core::abgr_to_argb(cfg.color));
	draw_labels(labels, x0, baseline, scale, gap);

	gs_technique_end_pass(tech_draw_);
	gs_technique_end(tech_draw_);
	gs_blend_state_pop();
}

} // namespace topscreen
