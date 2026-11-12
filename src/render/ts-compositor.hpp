#pragma once

#include <graphics/graphics.h>
#include <obs-module.h>

#include <cstdint>
#include <vector>

#include "core/config.hpp"
#include "core/cursor_atlas.hpp"
#include "render/ts-frame.hpp"
#include "render/ts-text.hpp"
#include "render/ts-wallpaper.hpp"

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

	void render(obs_source_t *filter, obs_source_t *target, uint32_t width, uint32_t height,
		    const core::Config &cfg, const FrameState &st);

	bool loaded() const { return ready_; }

private:
	void cache_params();
	bool validate_params() const;
	void load_cursor_atlas();
	void release_cursor_atlas();
	void upload_background(const core::Config &cfg, uint32_t w, uint32_t h, bool active);
	void upload_cursor(const core::Config &cfg, const FrameState &st, bool active);
	void upload_focus(const core::Config &cfg, const FrameState &st, bool active);
	void upload_highlight(const FrameState &st, bool active);
	void upload_clicks(const FrameState &st);
	void upload_line(const FrameState &st, uint32_t w, bool active);


	gs_effect_t *effect_ = nullptr;
	/* True only when the effect compiled AND every critical uniform resolved;
	 * otherwise the filter stays a passthrough instead of drawing nothing. */
	bool ready_ = false;
	bool logged_first_frame_ = false;
	bool logged_line_stroke_ = false;
	bool logged_begin_failure_ = false;
	/* Diagnostic: TOPSCREEN_TECH=passthrough draws only the source. */
	bool use_passthrough_ = false;

	gs_eparam_t *p_target_size_ = nullptr;
	gs_eparam_t *p_pad_ = nullptr;
	gs_eparam_t *p_radius_ = nullptr;
	gs_eparam_t *p_zoom_ = nullptr;
	gs_eparam_t *p_motion_blur_ = nullptr;
	gs_eparam_t *p_focus_ = nullptr;
	gs_eparam_t *p_bg_a_ = nullptr;
	gs_eparam_t *p_bg_b_ = nullptr;
	gs_eparam_t *p_bg_mode_ = nullptr;
	gs_eparam_t *p_bg_gradient_center_ = nullptr;
	gs_eparam_t *p_bg_gradient_dir_ = nullptr;
	gs_eparam_t *p_bg_tex_ = nullptr;
	gs_eparam_t *p_bg_tex_enabled_ = nullptr;
	gs_eparam_t *p_bg_tex_aspect_ = nullptr;
	gs_eparam_t *p_bg_transparent_ = nullptr;
	gs_eparam_t *p_bg_keep_aspect_ = nullptr;
	gs_eparam_t *p_bg_blur_ = nullptr;
	gs_eparam_t *p_frame_style_ = nullptr;
	gs_eparam_t *p_frame_color_ = nullptr;
	gs_eparam_t *p_frame_width_ = nullptr;
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

	gs_eparam_t *p_spotlight_enabled_ = nullptr;
	gs_eparam_t *p_spotlight_amount_ = nullptr;
	gs_eparam_t *p_spotlight_shape_ = nullptr;
	gs_eparam_t *p_spotlight_radius_ = nullptr;
	gs_eparam_t *p_spotlight_corner_ = nullptr;
	gs_eparam_t *p_spotlight_dim_ = nullptr;
	gs_eparam_t *p_spotlight_border_enabled_ = nullptr;
	gs_eparam_t *p_spotlight_border_color_ = nullptr;
	gs_eparam_t *p_spotlight_border_width_ = nullptr;
	gs_eparam_t *p_spotlight_outside_mode_ = nullptr;
	gs_eparam_t *p_spotlight_blur_ = nullptr;
	gs_eparam_t *p_spotlight_zoom_ = nullptr;
	gs_eparam_t *p_spotlight_focus_ = nullptr;
	gs_eparam_t *p_spotlight_zoom_focus_ = nullptr;

	gs_eparam_t *p_magnifier_enabled_ = nullptr;
	gs_eparam_t *p_magnifier_amount_ = nullptr;
	gs_eparam_t *p_magnifier_shape_ = nullptr;
	gs_eparam_t *p_magnifier_radius_ = nullptr;
	gs_eparam_t *p_magnifier_corner_ = nullptr;
	gs_eparam_t *p_magnifier_zoom_ = nullptr;
	gs_eparam_t *p_magnifier_focus_ = nullptr;
	gs_eparam_t *p_magnifier_sample_ = nullptr;
	gs_eparam_t *p_magnifier_border_enabled_ = nullptr;
	gs_eparam_t *p_magnifier_border_color_ = nullptr;
	gs_eparam_t *p_magnifier_border_width_ = nullptr;
	gs_eparam_t *p_magnifier_shadow_enabled_ = nullptr;
	gs_eparam_t *p_magnifier_shadow_color_ = nullptr;
	gs_eparam_t *p_magnifier_shadow_alpha_ = nullptr;
	gs_eparam_t *p_magnifier_shadow_blur_ = nullptr;
	gs_eparam_t *p_magnifier_shadow_offset_ = nullptr;

	gs_eparam_t *p_highlight_active_ = nullptr;
	gs_eparam_t *p_highlight_rect_ = nullptr;
	gs_eparam_t *p_highlight_color_ = nullptr;
	gs_eparam_t *p_highlight_alpha_ = nullptr;
	gs_eparam_t *p_reading_active_ = nullptr;
	gs_eparam_t *p_reading_rect_ = nullptr;
	gs_eparam_t *p_reading_dim_ = nullptr;

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

	/* Freehand line: up to 32 polyline points (line_p0..line_p31) and a point
	 * count. Zero-filled every frame so D3D11 never sees an unset uniform. */
	gs_eparam_t *p_line_pts_[32] = {nullptr};
	gs_eparam_t *p_line_count_ = nullptr;
	gs_eparam_t *p_line_width_ = nullptr;

	TextRenderer text_;

	/* Cursor atlas (style bitmaps). */
	gs_texture_t *cursor_tex_ = nullptr;
	/* 1x1 transparent texture: bound to unused texture uniforms so that every
	 * shader parameter is set (the D3D11 device refuses to draw otherwise). */
	gs_texture_t *dummy_tex_ = nullptr;
	core::CursorAtlas cursor_atlas_;
	std::vector<core::Vec2f> cursor_hotspots_;

	WallpaperPipeline wallpaper_;

	/* Gradient direction is only recomputed when the angle changes. The
	 * uniform itself is still uploaded every frame (D3D11 requirement). */
	bool gradient_dir_valid_ = false;
	double gradient_angle_ = 0.0;
	float gradient_dir_x_ = 1.0f;
	float gradient_dir_y_ = 0.0f;
};

} // namespace topscreen
