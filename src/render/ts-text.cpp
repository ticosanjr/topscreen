#include "ts-text.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>

#include <obs-module.h>

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

bool TextRenderer::load(const char *ttf_module_file, const char *effect_module_file)
{
	release();

	std::vector<uint8_t> ttf;
	if (!read_module_bytes(ttf_module_file, &ttf)) {
		blog(LOG_WARNING, "[TopScreen] fonte nao encontrada: %s", ttf_module_file);
		return false;
	}

	core::FontAtlasOptions opt;
	opt.pixel_height = 32;
	std::string err;
	if (!core::font_atlas_build(ttf.data(), ttf.size(), opt, &atlas_, &err)) {
		blog(LOG_WARNING, "[TopScreen] falha ao gerar o atlas da fonte: %s", err.c_str());
		return false;
	}

	char *fx_path = obs_module_file(effect_module_file);
	if (!fx_path) {
		blog(LOG_WARNING, "[TopScreen] efeito de texto nao encontrado: %s", effect_module_file);
		return false;
	}

	obs_enter_graphics();
	const uint8_t *data[1] = {atlas_.pixels.data()};
	texture_ = gs_texture_create(static_cast<uint32_t>(atlas_.width), static_cast<uint32_t>(atlas_.height),
				     GS_RGBA, 1, data, 0);
	effect_ = gs_effect_create_from_file(fx_path, nullptr);
	obs_leave_graphics();

	bfree(fx_path);

	if (!texture_ || !effect_) {
		blog(LOG_WARNING, "[TopScreen] falha ao criar textura/efeito de texto");
		release();
		return false;
	}

	p_image_ = gs_effect_get_param_by_name(effect_, "image");
	p_color_ = gs_effect_get_param_by_name(effect_, "color");
	return true;
}

void TextRenderer::release()
{
	if (texture_ || effect_) {
		obs_enter_graphics();
		if (texture_) {
			gs_texture_destroy(texture_);
			texture_ = nullptr;
		}
		if (effect_) {
			gs_effect_destroy(effect_);
			effect_ = nullptr;
		}
		obs_leave_graphics();
	}
	p_image_ = nullptr;
	p_color_ = nullptr;
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

void TextRenderer::draw(const core::KeyRender *items, int count, uint32_t target_w, uint32_t target_h)
{
	if (!loaded() || !items || count <= 0 || target_w == 0 || target_h == 0)
		return;

	std::vector<std::string> labels;
	labels.reserve(static_cast<size_t>(count));
	for (int i = 0; i < count; ++i)
		labels.push_back(upper_copy(items[i].label));

	const float scale = std::max(1.0f, static_cast<float>(target_h) * 0.035f / 32.0f);
	const float gap = 14.0f * scale;
	const float line_h = static_cast<float>(atlas_.line_height) * scale;

	float total = 0.0f;
	for (const std::string &s : labels)
		total += measure(s, scale);
	if (labels.size() > 1)
		total += gap * static_cast<float>(labels.size() - 1);

	const float x0 = (static_cast<float>(target_w) - total) * 0.5f;
	const float baseline = static_cast<float>(target_h) - line_h * 0.9f;

	gs_technique_t *tech = gs_effect_get_technique(effect_, "Draw");
	if (!tech)
		return;

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	if (p_image_)
		gs_effect_set_texture(p_image_, texture_);

	/* Contorno escuro (4 deslocamentos) */
	if (p_color_)
		gs_effect_set_color(p_color_, 0xFF000000);
	const float o = std::max(1.0f, 2.0f * scale);
	const float offsets[4][2] = {{-o, 0.0f}, {o, 0.0f}, {0.0f, -o}, {0.0f, o}};
	for (const auto &off : offsets)
		draw_labels(labels, x0 + off[0], baseline + off[1], scale, gap);

	/* Preenchimento claro */
	if (p_color_)
		gs_effect_set_color(p_color_, 0xFFFFFFFF);
	draw_labels(labels, x0, baseline, scale, gap);

	gs_technique_end_pass(tech);
	gs_technique_end(tech);
	gs_blend_state_pop();
}

} // namespace topscreen
