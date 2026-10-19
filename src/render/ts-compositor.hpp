#pragma once

#include <graphics/graphics.h>
#include <obs-module.h>

#include <vector>

#include "core/config.hpp"
#include "core/cursor_atlas.hpp"
#include "render/ts-frame.hpp"
#include "core/pen.hpp"
#include "render/ts-highlight.hpp"
#include "render/ts-text.hpp"

#include <string>

namespace topscreen {

/**
 * Compositor: owns the TopScreen GPU effect.
 *
 * Receives the parent source's frame via obs_source_process_filter_* and draws the
 * composition (background, corners, shadow, zoom/pan, cursor and click effects)
 * in a single pass.
 */
class Compositor {
public:
	Compositor() = default;
	~Compositor();

	Compositor(const Compositor &) = delete;
	Compositor &operator=(const Compositor &) = delete;

	bool load();
	void release();

	void render(obs_source_t *filter, const core::Config &cfg, const FrameState &st);

	bool loaded() const { return ready_; }

private:
	void cache_params();
	bool validate_params() const;
	void load_cursor_atlas();
	void release_cursor_atlas();
	void load_bg_image(const char *path);
	void release_bg_image();

	gs_effect_t *effect_ = nullptr;
	/* True only when the effect compiled AND every critical uniform resolved;
	 * otherwise the filter stays a passthrough instead of drawing nothing. */
	bool ready_ = false;
	bool logged_first_frame_ = false;
	bool logged_line_draw_ = false;
	bool logged_line_attempt_ = false;
	bool logged_begin_failure_ = false;
	/* Diagnostic: TOPSCREEN_TECH=passthrough draws only the source. */
	bool use_passthrough_ = false;

	gs_eparam_t *p_target_size_ = nullptr;
	gs_eparam_t *p_pad_ = nullptr;
	gs_eparam_t *p_radius_ = nullptr;
	gs_eparam_t *p_zoom_ = nullptr;
	gs_eparam_t *p_focus_ = nullptr;
	gs_eparam_t *p_bg_a_ = nullptr;
	gs_eparam_t *p_bg_b_ = nullptr;
	gs_eparam_t *p_bg_mode_ = nullptr;
	gs_eparam_t *p_bg_tex_ = nullptr;
	gs_eparam_t *p_bg_tex_enabled_ = nullptr;
	gs_eparam_t *p_bg_tex_aspect_ = nullptr;
	gs_eparam_t *p_bg_keep_aspect_ = nullptr;
	gs_eparam_t *p_bg_blur_ = nullptr;
	gs_eparam_t *p_bg_blur_type_ = nullptr;
	gs_eparam_t *p_shadow_alpha_ = nullptr;
	gs_eparam_t *p_shadow_blur_ = nullptr;
	gs_eparam_t *p_shadow_offset_ = nullptr;

	gs_eparam_t *p_cursor_enabled_ = nullptr;
	gs_eparam_t *p_cursor_pos_ = nullptr;
	gs_eparam_t *p_cursor_scale_ = nullptr;
	gs_eparam_t *p_cursor_opacity_ = nullptr;
	gs_eparam_t *p_cursor_rotation_ = nullptr;
	gs_eparam_t *p_cursor_shadow_ = nullptr;

	gs_eparam_t *p_cursor_tex_ = nullptr;
	gs_eparam_t *p_cursor_tex_enabled_ = nullptr;
	gs_eparam_t *p_cursor_cell_off_ = nullptr;
	gs_eparam_t *p_cursor_cell_scale_ = nullptr;
	gs_eparam_t *p_cursor_hotspot_ = nullptr;

	gs_eparam_t *p_zoom_blur_ = nullptr;
	gs_eparam_t *p_spotlight_enabled_ = nullptr;
	gs_eparam_t *p_spotlight_radius_ = nullptr;
	gs_eparam_t *p_spotlight_dim_ = nullptr;

	gs_eparam_t *p_highlight_active_ = nullptr;
	gs_eparam_t *p_highlight_rect_ = nullptr;
	gs_eparam_t *p_highlight_color_ = nullptr;
	gs_eparam_t *p_highlight_alpha_ = nullptr;

	/* Explicit click slots (the shader does not use uniform arrays). */
	struct ClickParams {
		gs_eparam_t *pos = nullptr;
		gs_eparam_t *prog = nullptr;
		gs_eparam_t *size = nullptr;
		gs_eparam_t *color = nullptr;
		gs_eparam_t *type = nullptr;
	};

	gs_eparam_t *p_click_count_ = nullptr;
	ClickParams click_[4];

	TextRenderer text_;
	HighlightRenderer highlight_;
	core::PenCanvas pen_;
	int pen_points_ = 0;

	/* Cursor atlas (style bitmaps). */
	gs_texture_t *cursor_tex_ = nullptr;
	/* 1x1 transparent texture: bound to unused texture uniforms so that every
	 * shader parameter is set (the D3D11 device refuses to draw otherwise). */
	gs_texture_t *dummy_tex_ = nullptr;
	core::CursorAtlas cursor_atlas_;
	std::vector<core::Vec2f> cursor_hotspots_;

	/* Background wallpaper. */
	gs_texture_t *bg_tex_ = nullptr;
	std::string bg_loaded_;
	std::string bg_requested_;
	float bg_aspect_ = 1.0f;
};

} // namespace topscreen
