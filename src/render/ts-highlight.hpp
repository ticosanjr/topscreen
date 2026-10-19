#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <graphics/graphics.h>

namespace topscreen {

/**
 * Freehand highlight overlay.
 *
 * Uploads the pen mask (8-bit alpha) to a texture and draws it tinted over a
 * rectangle, with the same lifecycle as the rest of the renderer classes.
 */
class HighlightRenderer {
public:
	HighlightRenderer() = default;
	~HighlightRenderer();

	HighlightRenderer(const HighlightRenderer &) = delete;
	HighlightRenderer &operator=(const HighlightRenderer &) = delete;

	bool load();
	void release();

	/** Uploads the mask when its revision changed. */
	void upload(int width, int height, const std::vector<uint8_t> &alpha, uint32_t revision);

	/** Draws the mask over (x,y,w,h) with the given color (0xAABBGGRR). */
	void draw(float x, float y, float w, float h, uint32_t color, float alpha);

	/** The effect is usable (a texture is created on the first upload). */
	bool ready() const { return effect_ != nullptr && tech_ != nullptr; }
	bool loaded() const { return ready() && texture_ != nullptr; }

private:
	gs_effect_t *effect_ = nullptr;
	gs_technique_t *tech_ = nullptr;
	gs_eparam_t *p_image_ = nullptr;
	gs_eparam_t *p_color_ = nullptr;
	std::vector<uint8_t> rgba_;
	gs_texture_t *texture_ = nullptr;
	uint32_t texture_revision_ = 0;
	int texture_w_ = 0;
	int texture_h_ = 0;
};

} // namespace topscreen
