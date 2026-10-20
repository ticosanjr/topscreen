#include "ts-compositor.hpp"

#include <graphics/vec2.h>
#include <graphics/vec4.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "core/color.hpp"
#include "core/cursor_atlas.hpp"
#include "core/cursor_catalog.hpp"
#include "core/file.hpp"
#include "core/image.hpp"
#include "core/types.hpp"
#include "ts-wallpapers.hpp"

namespace topscreen {

namespace {

/* Guarded setters: a missing uniform cannot crash OBS. */
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
		blog(LOG_ERROR, "[TopScreen] obs_module_file(effects/topscreen.effect) failed");
		return false;
	}

	obs_enter_graphics();
	effect_ = gs_effect_create_from_file(path, nullptr);
	obs_leave_graphics();

	bfree(path);

	if (!effect_) {
		blog(LOG_ERROR, "[TopScreen] failed to create the topscreen.effect effect");
		return false;
	}

	cache_params();

	/* Diagnostic switch (also handy for support): draw only the source. */
	if (const char *tech = std::getenv("TOPSCREEN_TECH"))
		use_passthrough_ = std::string(tech) == "passthrough";
	if (use_passthrough_)
		blog(LOG_WARNING, "[TopScreen] diagnostic mode: pass-through technique");

	/* Missing uniforms would leave the shader reading zeros (a fully
	 * transparent frame). Prefer a passthrough over an invisible source. */
	ready_ = validate_params();
	if (!ready_)
		blog(LOG_ERROR, "[TopScreen] shader is incomplete: the filter will pass "
				"the source through until this is fixed");

	/* Key overlay (best effort: if the font/effect is missing, proceeds without it). */
	text_.load("fonts/DejaVuSans-Bold.ttf", "effects/text.effect");

	/* Cursor atlas (best effort: without it, uses the procedural cursor). */
	load_cursor_atlas();

	/* 1x1 transparent texture for unused texture uniforms. */
	if (!dummy_tex_) {
		const uint8_t px[4] = {0, 0, 0, 0};
		const uint8_t *data[1] = {px};
		obs_enter_graphics();
		dummy_tex_ = gs_texture_create(1, 1, GS_RGBA, 1, data, 0);
		obs_leave_graphics();
	}

	return ready_;
}

void Compositor::load_cursor_atlas()
{
	char *path = obs_module_file("cursors/atlas.rgba");
	if (!path) {
		blog(LOG_WARNING, "[TopScreen] cursor atlas not found (using procedural)");
		return;
	}

	std::vector<uint8_t> bytes;
	const bool ok = core::read_file_bytes(path, &bytes);
	bfree(path);
	if (!ok)
		return;

	std::string err;
	if (!core::cursor_atlas_parse(bytes.data(), bytes.size(), &cursor_atlas_, &err)) {
		blog(LOG_WARNING, "[TopScreen] invalid cursor atlas: %s", err.c_str());
		return;
	}

	/* Hotspots (optional; if missing, uses a default). */
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
		blog(LOG_WARNING, "[TopScreen] failed to create the cursor atlas texture");
		return;
	}

	blog(LOG_INFO, "[TopScreen] cursor atlas: %u styles (%ux%u)", cursor_atlas_.count,
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

	/* Freehand line: 16 explicit points (line_p0..line_p15), no arrays. */
	for (int i = 0; i < 16; ++i) {
		std::snprintf(name, sizeof(name), "line_p%d", i);
		p_line_pts_[i] = P(name);
	}
	p_line_count_ = P("line_count");
	p_line_width_ = P("line_width");
}

bool Compositor::validate_params() const
{
	if (!effect_)
		return false;

	/* Every uniform the shader declares must resolve: on D3D11 a parameter
	 * that is looked up (or uploaded) by the wrong name aborts the draw and
	 * the filter's output stays empty. Keep this list in sync with
	 * data/effects/topscreen.effect (the harness test also checks it). */
	const char *const kNames[] = {
		"ViewProj",
		"image",
		"target_size",
		"content_pad",
		"corner_radius",
		"zoom",
		"zoom_focus",
		"bg_a",
		"bg_b",
		"bg_mode",
		"bg_tex",
		"bg_tex_enabled",
		"bg_tex_aspect",
		"bg_keep_aspect",
		"bg_blur",
		"bg_blur_type",
		"shadow_alpha",
		"shadow_blur",
		"shadow_offset",
		"cursor_enabled",
		"cursor_pos",
		"cursor_scale",
		"cursor_opacity",
		"cursor_rotation",
		"cursor_shadow",
		"cursor_tex",
		"cursor_tex_enabled",
		"cursor_cell_off",
		"cursor_cell_scale",
		"cursor_hotspot",
		"zoom_blur",
		"spotlight_enabled",
		"spotlight_radius",
		"spotlight_dim",
		"highlight_active",
		"highlight_rect",
		"highlight_color",
		"highlight_alpha",
		"click_count",
		"line_count",
		"line_width",
	};

	int missing = 0;
	for (const char *name : kNames) {
		if (!gs_effect_get_param_by_name(effect_, name)) {
			blog(LOG_ERROR, "[TopScreen] shader uniform missing: %s", name);
			++missing;
		}
	}
	for (int i = 0; i < 4; ++i) {
		for (const char *suffix : {"_pos", "_prog", "_size", "_color", "_type"}) {
			char name[32];
			std::snprintf(name, sizeof(name), "click%d%s", i, suffix);
			if (!gs_effect_get_param_by_name(effect_, name)) {
				blog(LOG_ERROR, "[TopScreen] shader uniform missing: %s", name);
				++missing;
			}
		}
	}

	for (int i = 0; i < 16; ++i) {
		char name[32];
		std::snprintf(name, sizeof(name), "line_p%d", i);
		if (!gs_effect_get_param_by_name(effect_, name)) {
			blog(LOG_ERROR, "[TopScreen] shader uniform missing: %s", name);
			++missing;
		}
	}

	/* The effect must expose the technique OBS draws with, and at least one
	 * pass; otherwise the draw is a no-op and the frame stays empty. */
	gs_technique_t *tech = gs_effect_get_technique(effect_, "Draw");
	if (!tech) {
		blog(LOG_ERROR, "[TopScreen] shader has no 'Draw' technique");
		++missing;
	} else {
		obs_enter_graphics();
		const size_t passes = gs_technique_begin(tech);
		gs_technique_end(tech);
		obs_leave_graphics();
		if (passes == 0) {
			blog(LOG_ERROR, "[TopScreen] shader 'Draw' technique has no passes");
			++missing;
		}
	}

	return missing == 0;
}

void Compositor::release()
{
	text_.release();
	release_cursor_atlas();
	release_bg_image();

	if (dummy_tex_) {
		obs_enter_graphics();
		gs_texture_destroy(dummy_tex_);
		obs_leave_graphics();
		dummy_tex_ = nullptr;
	}

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

	p_line_count_ = p_line_width_ = nullptr;
	for (auto *&pt : p_line_pts_)
		pt = nullptr;
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

	if (!ready_) {
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!obs_source_process_filter_begin(filter, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
		/* Nothing was drawn: without this the filter's output would be an
		 * empty frame (the source looks invisible). Pass it through and
		 * leave a trail in the log (warnings always show in the OBS log). */
		if (!logged_begin_failure_) {
			logged_begin_failure_ = true;
			blog(LOG_WARNING, "[TopScreen] process_filter_begin failed (%.0fx%.0f): "
					  "passing the source through without effects",
			     static_cast<double>(w), static_cast<double>(h));
		}
		obs_source_skip_video_filter(filter);
		return;
	}

	if (!logged_first_frame_) {
		logged_first_frame_ = true;
		blog(LOG_WARNING, "[TopScreen] first frame: target=%ux%u ready=%s technique=%s", w, h,
		     ready_ ? "yes" : "no", use_passthrough_ ? "PassThrough" : "Draw");
	}

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

	/* Wallpaper: if there is an image (from the PC or built-in), it is used.
	 * The path is passed as a C string so the hot path does not allocate. */
	load_bg_image(active ? core::resolve_bg_path(cfg.bg.image.c_str(), cfg.bg.image_file.c_str())
			     : "");
	const bool bgimg = bg && bg_tex_ != nullptr;
	/* D3D11 aborts the draw ("Not all shader parameters were set") when any
	 * scalar uniform was never uploaded, leaving the frame empty, so every
	 * uniform below is uploaded on EVERY frame, even with the feature off.
	 * Textures always get a real binding (a 1x1 dummy when unused). */
	set_f(p_bg_tex_enabled_, bgimg ? 1.0f : 0.0f);
	set_f(p_bg_blur_, static_cast<float>(cfg.bg.image_blur));
	set_f(p_bg_blur_type_, static_cast<float>(cfg.bg.image_blur_type));
	set_f(p_bg_tex_aspect_, bg_aspect_);
	if (p_bg_tex_)
		gs_effect_set_texture(p_bg_tex_, bgimg && bg_tex_ ? bg_tex_ : dummy_tex_);

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

	/* Cursor bitmap (atlas). Always uploaded (see the D3D11 note above);
	 * a 1x1 dummy texture is bound when there is no atlas. */
	const bool has_atlas = (cursor_tex_ != nullptr && cursor_atlas_.count > 0);
	set_f(p_cursor_tex_enabled_, has_atlas ? 1.0f : 0.0f);
	{
		const uint32_t idx =
			has_atlas ? std::min<uint32_t>(
					    static_cast<uint32_t>(core::cursor_style_clamp(cfg.cursor.style)),
					    cursor_atlas_.count - 1)
				  : 0u;
		const float n = has_atlas ? static_cast<float>(cursor_atlas_.count) : 1.0f;
		set_v2(p_cursor_cell_off_, 0.0f, static_cast<float>(idx) / n);
		set_v2(p_cursor_cell_scale_, 1.0f, 1.0f / n);

		core::Vec2f hs{0.05, 0.05};
		if (has_atlas && idx < cursor_hotspots_.size())
			hs = cursor_hotspots_[idx];
		set_v2(p_cursor_hotspot_, static_cast<float>(hs.x), static_cast<float>(hs.y));

		if (p_cursor_tex_)
			gs_effect_set_texture(p_cursor_tex_, has_atlas ? cursor_tex_ : dummy_tex_);
	}

	/* Zoom motion blur */
	set_f(p_zoom_blur_, active ? static_cast<float>(st.zoom_blur) : 0.0f);

	/* Spotlight */
	const bool spot = active && cfg.spotlight.enabled;
	set_f(p_spotlight_enabled_, spot ? 1.0f : 0.0f);
	set_f(p_spotlight_radius_, static_cast<float>(cfg.spotlight.radius));
	set_f(p_spotlight_dim_, static_cast<float>(cfg.spotlight.dim));

	/* Drag highlighter: only the box mode paints the rectangle. The line is
	 * drawn by its own overlay and the zoom gesture draws nothing. */
	const bool hl = active && core::paints_box(st.highlight);
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

	/* Clicks (up to kShaderClickSlots, explicit slots). Every slot is
	 * uploaded on every frame, zeroed when unused: the D3D11 draw aborts if
	 * a slot uniform was never set. */
	int used = st.click_count;
	if (used > kShaderClickSlots)
		used = kShaderClickSlots;
	set_f(p_click_count_, static_cast<float>(used));

	for (int i = 0; i < kShaderClickSlots; ++i) {
		if (i < used) {
			const core::ClickRender &c = st.clicks[i];
			set_v2(click_[i].pos, static_cast<float>(c.x), static_cast<float>(c.y));
			set_f(click_[i].prog, static_cast<float>(c.progress));
			set_f(click_[i].size, static_cast<float>(c.size));
			set_c(click_[i].color, c.color);
			set_f(click_[i].type, static_cast<float>(c.effect));
		} else {
			set_v2(click_[i].pos, 0.0f, 0.0f);
			set_f(click_[i].prog, 0.0f);
			set_f(click_[i].size, 0.0f);
			set_c(click_[i].color, 0);
			set_f(click_[i].type, 0.0f);
		}
	}

	/* Freehand line (Line / Line + zoom): the trail is decimated into at most
	 * 16 points and drawn by the SAME pass as everything else (segments in
	 * the shader), so no separate effect/texture/draw call can fail on its
	 * own. Every slot is uploaded on every frame, zeroed when unused (see
	 * the D3D11 note above); the active stroke keeps the model's fade
	 * alpha. */
	const bool line_on = active && core::draws_line(st.highlight) && st.highlight.point_count > 1;
	const int src_pts = st.highlight.point_count;
	core::Vec2f line_pts[kShaderLineSlots];
	const int line_count =
		line_on ? core::subsample_line(st.highlight.points, src_pts, line_pts,
					       kShaderLineSlots)
			: 0;
	for (int i = 0; i < kShaderLineSlots; ++i) {
		if (i < line_count)
			set_v2(p_line_pts_[i], line_pts[i].x, line_pts[i].y);
		else
			set_v2(p_line_pts_[i], 0.0f, 0.0f);
	}
	set_f(p_line_count_, static_cast<float>(line_count));
	float line_w = static_cast<float>(st.highlight.thickness) * static_cast<float>(w);
	if (line_w < 2.0f)
		line_w = 2.0f;
	set_f(p_line_width_, line_w);

	if (line_count > 1 && !logged_line_stroke_) {
		logged_line_stroke_ = true;
		blog(LOG_INFO,
		     "[TopScreen] line highlight: %d of %d points drawn as %d shader segments (width %.1fpx)",
		     line_count, src_pts, line_count - 1, static_cast<double>(line_w));
	}

	if (use_passthrough_)
		obs_source_process_filter_tech_end(filter, effect_, w, h, "PassThrough");
	else
		obs_source_process_filter_end(filter, effect_, w, h);

	/* Key overlay on top of the result. */
	if (active && cfg.keys.enabled && st.key_count > 0)
		text_.draw(st.keys, st.key_count, cfg.keys, w, h);
}

void Compositor::load_bg_image(const char *raw)
{
	const char *requested = raw ? raw : "";
	/* The requested path rarely changes: compare without building strings. */
	if (bg_requested_ == requested)
		return;
	bg_requested_ = requested;

	/* Auto-correction: a built-in wallpaper name from older configs
	 * (e.g. "Aurora") becomes its path. */
	std::string path = requested;
	if (const char *w = wallpaper_path_for(requested))
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
			blog(LOG_WARNING, "[TopScreen] wallpaper not found: %s", path.c_str());
			return;
		}
		const bool mok = core::read_file_bytes(m, &bytes);
		bfree(m);
		if (!mok)
			bytes.clear();
	}

	if (bytes.empty()) {
		blog(LOG_WARNING, "[TopScreen] empty/unreadable wallpaper: %s", path.c_str());
		return;
	}

	core::Image img;
	std::string err;
	if (!core::image_decode(bytes.data(), bytes.size(), &img, &err)) {
		blog(LOG_WARNING, "[TopScreen] invalid wallpaper (%s): %s", path.c_str(), err.c_str());
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
		blog(LOG_WARNING, "[TopScreen] failed to create the wallpaper texture: %s", path.c_str());
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
