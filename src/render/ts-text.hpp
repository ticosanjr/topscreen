#pragma once

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
	bool reload_font(const std::string &module_file);
	float measure(const std::string &text, float scale) const;
	void draw_string(const std::string &text, float x, float baseline, float scale);
	void draw_labels(const std::vector<std::string> &labels, const std::vector<float> &widths, float x0,
			 float baseline, float scale, float gap);
	void draw_band(float x, float y, float w, float h, float radius, uint32_t color);

	core::FontAtlas atlas_;
	std::string font_path_;
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
