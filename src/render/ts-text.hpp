#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <graphics/graphics.h>

#include "core/font_atlas.hpp"
#include "core/key_overlay.hpp"

namespace topscreen {

/**
 * Key overlay renderer.
 *
 * Builds the atlas from a TTF (selectable), creates the textures and draws
 * the background band + shadow + text, all configurable.
 */
class TextRenderer {
public:
	TextRenderer() = default;
	~TextRenderer();

	TextRenderer(const TextRenderer &) = delete;
	TextRenderer &operator=(const TextRenderer &) = delete;

	bool load(const char *ttf_module_file, const char *effect_module_file);
	void release();

	void draw(const core::KeyRender *items, int count, const core::KeyOverlayConfig &cfg, uint32_t target_w,
		  uint32_t target_h);

	bool loaded() const { return texture_ != nullptr && effect_ != nullptr; }

private:
	static constexpr int kMaxLabels = 8;
	static constexpr int kMaxLabelBytes = 63;
	static constexpr float kGapUnits = 14.0f;

	struct Run {
		int first = 0;
		int count = 0;
		float width = 0.0f; // font units
	};

	bool reload_font(const std::string &module_file);
	bool build_layout(const core::KeyRender *items, int count);
	void rebuild_geometry();
	void destroy_geometry();
	void draw_geometry(float x, float baseline, float scale, uint32_t color);
	void draw_geometry_fallback(float x, float baseline, float scale, uint32_t color);
	void draw_band(float x, float y, float w, float h, float radius, uint32_t color);

	core::FontAtlas atlas_;
	std::string font_path_;
	const core::Glyph *fallback_glyph_ = nullptr;

	/* Key-overlay text is stable for seconds, so the decoded glyph runs and
	 * the vertex buffer survive across frames. */
	std::array<core::KeyLabel, kMaxLabels> cached_labels_;
	int cached_count_ = 0;
	bool layout_valid_ = false;
	std::vector<const core::Glyph *> layout_glyphs_;
	std::array<Run, kMaxLabels> runs_{};
	int run_count_ = 0;
	bool geometry_with_shadow_ = false;

	/* All glyph quads for the current text, in font units. */
	gs_vertbuffer_t *glyph_vb_ = nullptr;
	uint32_t glyph_vertex_count_ = 0;

	gs_texture_t *texture_ = nullptr;
	gs_texture_t *white_ = nullptr;
	gs_effect_t *effect_ = nullptr;
	gs_technique_t *tech_draw_ = nullptr;
	gs_technique_t *tech_band_ = nullptr;
	gs_eparam_t *p_image_ = nullptr;
	gs_eparam_t *p_color_ = nullptr;
	gs_eparam_t *p_band_size_ = nullptr;
	gs_eparam_t *p_band_radius_ = nullptr;
	gs_eparam_t *p_band_color_ = nullptr;
};

} // namespace topscreen
