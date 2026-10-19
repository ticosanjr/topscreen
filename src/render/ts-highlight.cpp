#include "ts-highlight.hpp"

#include <obs-module.h>

#include <algorithm>

#include "core/color.hpp"

namespace topscreen {

HighlightRenderer::~HighlightRenderer()
{
	release();
}

bool HighlightRenderer::load()
{
	if (effect_)
		return true;

	char *path = obs_module_file("effects/highlight.effect");
	if (!path)
		return false;

	obs_enter_graphics();
	effect_ = gs_effect_create_from_file(path, nullptr);
	obs_leave_graphics();

	bfree(path);

	if (!effect_) {
		blog(LOG_WARNING, "[TopScreen] highlight effect not available");
		return false;
	}

	tech_ = gs_effect_get_technique(effect_, "Draw");
	p_image_ = gs_effect_get_param_by_name(effect_, "image");
	p_color_ = gs_effect_get_param_by_name(effect_, "color");
	if (!tech_ || !p_image_ || !p_color_) {
		blog(LOG_WARNING, "[TopScreen] highlight effect is incomplete");
		release();
		return false;
	}
	return true;
}

void HighlightRenderer::release()
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
	tech_ = nullptr;
	p_image_ = p_color_ = nullptr;
	texture_revision_ = 0;
	texture_w_ = texture_h_ = 0;
}

void HighlightRenderer::upload(int width, int height, const std::vector<uint8_t> &alpha,
			       uint32_t revision)
{
	if (!effect_ || width <= 0 || height <= 0 || alpha.empty())
		return;
	if (revision == texture_revision_)
		return;

	/* RGBA8 with white RGB and the mask in the alpha channel: the most
	 * portable texture format on both D3D11 and OpenGL. */
	const size_t pixels = static_cast<size_t>(width) * static_cast<size_t>(height);
	rgba_.resize(pixels * 4u);
	for (size_t i = 0; i < pixels; ++i) {
		rgba_[i * 4u + 0] = 255;
		rgba_[i * 4u + 1] = 255;
		rgba_[i * 4u + 2] = 255;
		rgba_[i * 4u + 3] = alpha[i];
	}

	obs_enter_graphics();
	bool ok = true;
	if (!texture_ || width != texture_w_ || height != texture_h_) {
		if (texture_) {
			gs_texture_destroy(texture_);
			texture_ = nullptr;
		}
		const uint8_t *data[1] = {rgba_.data()};
		texture_ = gs_texture_create(static_cast<uint32_t>(width), static_cast<uint32_t>(height),
					     GS_RGBA, 1, data, 0);
		ok = texture_ != nullptr;
		texture_w_ = width;
		texture_h_ = height;
	} else {
		gs_texture_set_image(texture_, rgba_.data(), static_cast<uint32_t>(width) * 4u, false);
	}
	obs_leave_graphics();

	/* Only advance on success: a failed upload must be retried next frame
	 * instead of freezing the stroke forever. */
	if (ok) {
		texture_revision_ = revision;
	} else if (!warned_upload_) {
		warned_upload_ = true;
		blog(LOG_WARNING, "[TopScreen] line highlight: texture upload failed (%dx%d)",
		     width, height);
	}
}

void HighlightRenderer::draw(float x, float y, float w, float h, uint32_t color, float alpha)
{
	if (!ready() || !texture_)
		return;

	/* The fade scales the color's alpha, not the mask. */
	if (p_color_) {
		uint32_t argb = core::abgr_to_argb(color);
		const auto a = static_cast<uint32_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f) << 24;
		gs_effect_set_color(p_color_, (argb & 0x00FFFFFFu) | a);
	}
	if (p_image_)
		gs_effect_set_texture(p_image_, texture_);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
	gs_technique_begin(tech_);
	gs_technique_begin_pass(tech_, 0);
	gs_matrix_push();
	gs_matrix_translate3f(x, y, 0.0f);
	gs_matrix_scale3f(w, h, 1.0f);
	gs_draw_sprite_subregion(texture_, 0, 0, 0, static_cast<uint32_t>(texture_w_),
				 static_cast<uint32_t>(texture_h_));
	gs_matrix_pop();
	gs_technique_end_pass(tech_);
	gs_technique_end(tech_);
	gs_blend_state_pop();
}

} // namespace topscreen
