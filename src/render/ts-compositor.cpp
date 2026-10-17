#include "ts-compositor.hpp"

#include <graphics/vec2.h>
#include <graphics/vec4.h>

#include <algorithm>
#include <cstdio>
#include <vector>

#include "core/color.hpp"
#include "core/cursor_atlas.hpp"
#include "core/file.hpp"
#include "core/image.hpp"
#include "core/types.hpp"
#include "ts-wallpapers.hpp"

namespace topscreen {

namespace {

/* Setters com guarda: um uniform ausente nao pode derrubar o OBS. */
inline void set_f(gs_eparam_t *p, float v)
{
	if (p)
		gs_effect_set_float(p, v);
}
inline void set_v2(gs_eparam_t *p, float x, float y)
{
	if (!p)
		return;
	struct vec2 v;
	vec2_set(&v, x, y);
	gs_effect_set_vec2(p, &v);
}
inline void set_c(gs_eparam_t *p, uint32_t abgr)
{
	if (p)
		gs_effect_set_color(p, core::abgr_to_argb(abgr));
}

} // namespace

Compositor::~Compositor()
{
	release();
}

bool Compositor::load()
{
	if (effect_)
		return true;

	char *path = obs_module_file("effects/topscreen.effect");
	if (!path) {
		blog(LOG_ERROR, "[TopScreen] obs_module_file(effects/topscreen.effect) falhou");
		return false;
	}

	obs_enter_graphics();
	effect_ = gs_effect_create_from_file(path, nullptr);
	obs_leave_graphics();

	bfree(path);

	if (!effect_) {
		blog(LOG_ERROR, "[TopScreen] falha ao criar o efeito topscreen.effect");
		return false;
	}

	cache_params();
	validate_params();

	/* Overlay de teclas (melhor esforco: se faltar fonte/efeito, segue sem). */
	text_.load("fonts/DejaVuSans-Bold.ttf", "effects/text.effect");

	/* Atlas de cursores (melhor esforco: sem ele, usa o cursor procedural). */
	load_cursor_atlas();

	return true;
}

void Compositor::load_cursor_atlas()
{
	char *path = obs_module_file("cursors/atlas.rgba");
	if (!path) {
		blog(LOG_WARNING, "[TopScreen] atlas de cursores nao encontrado (usando procedural)");
		return;
	}

	std::vector<uint8_t> bytes;
	const bool ok = core::read_file_bytes(path, &bytes);
	bfree(path);
	if (!ok)
		return;

	std::string err;
	if (!core::cursor_atlas_parse(bytes.data(), bytes.size(), &cursor_atlas_, &err)) {
		blog(LOG_WARNING, "[TopScreen] atlas de cursores invalido: %s", err.c_str());
		return;
	}

	/* Hotspots (opcional; se faltar, usa um padrao). */
	char *jpath = obs_module_file("cursors/atlas.json");
	if (jpath) {
		std::string text;
		if (core::read_file_text(jpath, &text)) {
			std::string jerr;
			core::cursor_hotspots_parse(text, &cursor_hotspots_, &jerr);
		}
		bfree(jpath);
	}

	obs_enter_graphics();
	const uint8_t *data[1] = {cursor_atlas_.pixels.data()};
	cursor_tex_ = gs_texture_create(cursor_atlas_.width(), cursor_atlas_.height(), GS_RGBA, 1, data, 0);
	obs_leave_graphics();

	if (!cursor_tex_) {
		blog(LOG_WARNING, "[TopScreen] falha ao criar a textura do atlas de cursores");
		return;
	}

	blog(LOG_INFO, "[TopScreen] atlas de cursores: %u estilos (%ux%u)", cursor_atlas_.count,
	     cursor_atlas_.width(), cursor_atlas_.height());
}

void Compositor::release_cursor_atlas()
{
	if (cursor_tex_) {
		obs_enter_graphics();
		gs_texture_destroy(cursor_tex_);
		obs_leave_graphics();
		cursor_tex_ = nullptr;
	}
}

void Compositor::cache_params()
{
	if (!effect_)
		return;

	auto P = [this](const char *name) { return gs_effect_get_param_by_name(effect_, name); };

	p_target_size_ = P("target_size");
	p_pad_ = P("content_pad");
	p_radius_ = P("corner_radius");
	p_zoom_ = P("zoom");
	p_focus_ = P("zoom_focus");
	p_bg_a_ = P("bg_a");
	p_bg_b_ = P("bg_b");
	p_bg_mode_ = P("bg_mode");
	p_bg_tex_ = P("bg_tex");
	p_bg_tex_enabled_ = P("bg_tex_enabled");
	p_bg_tex_aspect_ = P("bg_tex_aspect");
	p_bg_keep_aspect_ = P("bg_keep_aspect");
	p_bg_blur_ = P("bg_blur");
	p_bg_blur_type_ = P("bg_blur_type");
	p_shadow_alpha_ = P("shadow_alpha");
	p_shadow_blur_ = P("shadow_blur");
	p_shadow_offset_ = P("shadow_offset");

	p_cursor_enabled_ = P("cursor_enabled");
	p_cursor_pos_ = P("cursor_pos");
	p_cursor_scale_ = P("cursor_scale");
	p_cursor_opacity_ = P("cursor_opacity");
	p_cursor_rotation_ = P("cursor_rotation");
	p_cursor_shadow_ = P("cursor_shadow");

	p_cursor_tex_ = P("cursor_tex");
	p_cursor_tex_enabled_ = P("cursor_tex_enabled");
	p_cursor_cell_off_ = P("cursor_cell_off");
	p_cursor_cell_scale_ = P("cursor_cell_scale");
	p_cursor_hotspot_ = P("cursor_hotspot");

	p_zoom_blur_ = P("zoom_blur");
	p_spotlight_enabled_ = P("spotlight_enabled");
	p_spotlight_radius_ = P("spotlight_radius");
	p_spotlight_dim_ = P("spotlight_dim");

	p_highlight_active_ = P("highlight_active");
	p_highlight_rect_ = P("highlight_rect");
	p_highlight_color_ = P("highlight_color");
	p_highlight_alpha_ = P("highlight_alpha");

	p_click_count_ = P("click_count");

	char name[32];
	for (int i = 0; i < 4; ++i) {
		std::snprintf(name, sizeof(name), "click%d_pos", i);
		click_[i].pos = P(name);
		std::snprintf(name, sizeof(name), "click%d_prog", i);
		click_[i].prog = P(name);
		std::snprintf(name, sizeof(name), "click%d_size", i);
		click_[i].size = P(name);
		std::snprintf(name, sizeof(name), "click%d_color", i);
		click_[i].color = P(name);
		std::snprintf(name, sizeof(name), "click%d_type", i);
		click_[i].type = P(name);
	}
}

void Compositor::validate_params() const
{
	if (!effect_)
		return;

	struct Named {
		const char *name;
		gs_eparam_t *param;
	};
	const Named checks[] = {
		{"target_size", p_target_size_},   {"content_pad", p_pad_},
		{"corner_radius", p_radius_},      {"zoom", p_zoom_},
		{"zoom_focus", p_focus_},          {"bg_a", p_bg_a_},
		{"bg_b", p_bg_b_},                 {"bg_mode", p_bg_mode_},
		{"cursor_enabled", p_cursor_enabled_}, {"cursor_pos", p_cursor_pos_},
		{"cursor_scale", p_cursor_scale_}, {"click_count", p_click_count_},
	};

	for (const Named &n : checks) {
		if (!n.param)
			blog(LOG_WARNING, "[TopScreen] uniform ausente no shader: %s", n.name);
	}
}

void Compositor::release()
{
	text_.release();
	release_cursor_atlas();
	release_bg_image();

	if (!effect_)
		return;

	obs_enter_graphics();
	gs_effect_destroy(effect_);
	obs_leave_graphics();

	effect_ = nullptr;
	p_target_size_ = p_pad_ = p_radius_ = p_zoom_ = p_focus_ = nullptr;
	p_bg_a_ = p_bg_b_ = p_bg_mode_ = nullptr;
	p_bg_tex_ = p_bg_tex_enabled_ = p_bg_tex_aspect_ = nullptr;
	p_bg_keep_aspect_ = nullptr;
	p_bg_blur_ = nullptr;
	p_bg_blur_type_ = nullptr;
	p_shadow_alpha_ = p_shadow_blur_ = p_shadow_offset_ = nullptr;
	p_cursor_enabled_ = p_cursor_pos_ = p_cursor_scale_ = nullptr;
	p_cursor_opacity_ = p_cursor_rotation_ = p_cursor_shadow_ = nullptr;
	p_zoom_blur_ = p_spotlight_enabled_ = p_spotlight_radius_ = p_spotlight_dim_ = nullptr;
	p_highlight_active_ = p_highlight_rect_ = p_highlight_color_ = p_highlight_alpha_ = nullptr;
	p_click_count_ = nullptr;
	for (auto &c : click_)
		c = ClickParams{};
}

void Compositor::render(obs_source_t *filter, const core::Config &cfg, const FrameState &st)
{
	obs_source_t *target = obs_filter_get_target(filter);
	if (!target) {
		obs_source_skip_video_filter(filter);
		return;
	}

	const uint32_t w = obs_source_get_base_width(target);
	const uint32_t h = obs_source_get_base_height(target);
	if (!w || !h) {
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!effect_) {
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!obs_source_process_filter_begin(filter, GS_RGBA, OBS_NO_DIRECT_RENDERING))
		return;

	const bool active = cfg.effects_enabled;
	const bool bg = active && cfg.bg.enabled;

	set_v2(p_target_size_, static_cast<float>(w), static_cast<float>(h));

	set_v2(p_pad_, bg ? static_cast<float>(cfg.bg.padding_x) : 0.0f,
	       bg ? static_cast<float>(cfg.bg.padding_y) : 0.0f);
	set_f(p_radius_, bg ? static_cast<float>(cfg.bg.corner_radius) : 0.0f);
	set_f(p_bg_mode_, bg ? static_cast<float>(cfg.bg.mode) : 0.0f);
	set_f(p_bg_keep_aspect_, cfg.bg.keep_aspect ? 1.0f : 0.0f);
	set_c(p_bg_a_, cfg.bg.color_a);
	set_c(p_bg_b_, cfg.bg.color_b);

	/* Wallpaper: se houver imagem (do PC ou embutida), ela e usada. */
	const std::string bg_path =
		core::resolve_bg_path(cfg.bg.image.c_str(), cfg.bg.image_file.c_str());
	load_bg_image(active ? bg_path : std::string());
	const bool bgimg = bg && bg_tex_ != nullptr;
	set_f(p_bg_tex_enabled_, bgimg ? 1.0f : 0.0f);
	set_f(p_bg_blur_, static_cast<float>(cfg.bg.image_blur));
	set_f(p_bg_blur_type_, static_cast<float>(cfg.bg.image_blur_type));
	if (bgimg) {
		set_f(p_bg_tex_aspect_, bg_aspect_);
		if (p_bg_tex_)
			gs_effect_set_texture(p_bg_tex_, bg_tex_);
	}

	const float shadow_alpha = (bg && cfg.bg.shadow) ? static_cast<float>(cfg.bg.shadow_alpha) : 0.0f;
	set_f(p_shadow_alpha_, shadow_alpha);
	set_f(p_shadow_blur_, static_cast<float>(cfg.bg.shadow_blur));
	set_v2(p_shadow_offset_, 0.0f, static_cast<float>(cfg.bg.shadow_offset));

	float zoom = active ? static_cast<float>(st.zoom.zoom) : 1.0f;
	if (zoom < 1.0f)
		zoom = 1.0f;
	set_f(p_zoom_, zoom);
	set_v2(p_focus_, static_cast<float>(st.zoom.focus_x), static_cast<float>(st.zoom.focus_y));

	/* Cursor */
	const bool cursor_on = active && cfg.cursor.enabled && st.cursor.visible && st.cursor.opacity > 0.001;
	set_f(p_cursor_enabled_, cursor_on ? 1.0f : 0.0f);
	set_v2(p_cursor_pos_, static_cast<float>(st.cursor.x), static_cast<float>(st.cursor.y));
	set_f(p_cursor_scale_, static_cast<float>(st.cursor.size));
	set_f(p_cursor_opacity_, static_cast<float>(st.cursor.opacity));
	set_f(p_cursor_rotation_, static_cast<float>(st.cursor.rotation));
	set_f(p_cursor_shadow_, cfg.cursor.shadow ? 1.0f : 0.0f);

	/* Cursor por bitmap (atlas). */
	const bool has_atlas = (cursor_tex_ != nullptr && cursor_atlas_.count > 0);
	set_f(p_cursor_tex_enabled_, has_atlas ? 1.0f : 0.0f);
	if (has_atlas) {
		const uint32_t idx =
			std::min<uint32_t>(static_cast<uint32_t>(cfg.cursor.style), cursor_atlas_.count - 1);
		const float n = static_cast<float>(cursor_atlas_.count);
		set_v2(p_cursor_cell_off_, 0.0f, static_cast<float>(idx) / n);
		set_v2(p_cursor_cell_scale_, 1.0f, 1.0f / n);

		core::Vec2f hs{0.05, 0.05};
		if (idx < cursor_hotspots_.size())
			hs = cursor_hotspots_[idx];
		set_v2(p_cursor_hotspot_, static_cast<float>(hs.x), static_cast<float>(hs.y));

		if (p_cursor_tex_)
			gs_effect_set_texture(p_cursor_tex_, cursor_tex_);
	}

	/* Motion blur do zoom */
	set_f(p_zoom_blur_, active ? static_cast<float>(st.zoom_blur) : 0.0f);

	/* Spotlight */
	const bool spot = active && cfg.spotlight.enabled;
	set_f(p_spotlight_enabled_, spot ? 1.0f : 0.0f);
	set_f(p_spotlight_radius_, static_cast<float>(cfg.spotlight.radius));
	set_f(p_spotlight_dim_, static_cast<float>(cfg.spotlight.dim));

	/* Marca-texto de arrasto */
	const bool hl = active && st.highlight.active;
	set_f(p_highlight_active_, hl ? 1.0f : 0.0f);
	{
		struct vec4 rect;
		vec4_set(&rect, static_cast<float>(st.highlight.x0), static_cast<float>(st.highlight.y0),
			 static_cast<float>(st.highlight.x1), static_cast<float>(st.highlight.y1));
		if (p_highlight_rect_)
			gs_effect_set_vec4(p_highlight_rect_, &rect);
	}
	set_c(p_highlight_color_, st.highlight.color);
	set_f(p_highlight_alpha_, static_cast<float>(st.highlight.alpha));

	/* Cliques (ate kShaderClickSlots, slots explicitos) */
	int used = st.click_count;
	if (used > kShaderClickSlots)
		used = kShaderClickSlots;
	set_f(p_click_count_, static_cast<float>(used));

	for (int i = 0; i < used; ++i) {
		const core::ClickRender &c = st.clicks[i];
		set_v2(click_[i].pos, static_cast<float>(c.x), static_cast<float>(c.y));
		set_f(click_[i].prog, static_cast<float>(c.progress));
		set_f(click_[i].size, static_cast<float>(c.size));
		set_c(click_[i].color, c.color);
		set_f(click_[i].type, static_cast<float>(c.effect));
	}

	obs_source_process_filter_end(filter, effect_, w, h);

	/* Overlay de teclas por cima do resultado. */
	if (active && cfg.keys.enabled && st.key_count > 0)
		text_.draw(st.keys, st.key_count, cfg.keys, w, h);
}

void Compositor::load_bg_image(const std::string &raw)
{
	/* O caminho pedido raramente muda; evita re-resolver por frame. */
	if (raw == bg_requested_)
		return;
	bg_requested_ = raw;

	/* Auto-correcao: se o valor for o nome de um wallpaper embutido
	 * (ex.: "Aurora", de versoes/config antigas), converte para o caminho. */
	std::string path = raw;
	if (const char *w = wallpaper_path_for(path.c_str()))
		path = w;

	if (path == bg_loaded_)
		return;

	if (bg_tex_) {
		obs_enter_graphics();
		gs_texture_destroy(bg_tex_);
		obs_leave_graphics();
		bg_tex_ = nullptr;
	}
	bg_loaded_ = path;

	if (path.empty())
		return;

	std::vector<uint8_t> bytes;
	if (!core::read_file_bytes(path.c_str(), &bytes)) {
		char *m = obs_module_file(path.c_str());
		if (!m) {
			blog(LOG_WARNING, "[TopScreen] wallpaper nao encontrado: %s", path.c_str());
			return;
		}
		const bool mok = core::read_file_bytes(m, &bytes);
		bfree(m);
		if (!mok)
			bytes.clear();
	}

	if (bytes.empty()) {
		blog(LOG_WARNING, "[TopScreen] wallpaper vazio/ilegivel: %s", path.c_str());
		return;
	}

	core::Image img;
	std::string err;
	if (!core::image_decode(bytes.data(), bytes.size(), &img, &err)) {
		blog(LOG_WARNING, "[TopScreen] wallpaper invalido (%s): %s", path.c_str(), err.c_str());
		return;
	}

	bg_aspect_ = static_cast<float>(img.width) / static_cast<float>(img.height);

	obs_enter_graphics();
	const uint8_t *data[1] = {img.pixels.data()};
	bg_tex_ = gs_texture_create(static_cast<uint32_t>(img.width), static_cast<uint32_t>(img.height), GS_RGBA, 1,
				    data, 0);
	obs_leave_graphics();

	if (bg_tex_)
		blog(LOG_INFO, "[TopScreen] wallpaper: %s (%dx%d)", path.c_str(), img.width, img.height);
	else
		blog(LOG_WARNING, "[TopScreen] falha ao criar a textura do wallpaper: %s", path.c_str());
}

void Compositor::release_bg_image()
{
	if (bg_tex_) {
		obs_enter_graphics();
		gs_texture_destroy(bg_tex_);
		obs_leave_graphics();
		bg_tex_ = nullptr;
	}
	bg_loaded_.clear();
	bg_requested_.clear();
}

} // namespace topscreen
