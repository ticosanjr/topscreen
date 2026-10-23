#pragma once

#include "image.hpp"

#include <cstdint>

namespace topscreen::core {

/** Bilinear RGBA resize. */
Image image_resize_rgba(const Image &src, int dst_width, int dst_height);

/** Downscale only when a source dimension exceeds max_dim (preserves aspect). */
Image image_limit_max(const Image &src, int max_dim);

/**
 * Static wallpaper prefilter.
 *
 * Large blurs are produced at half resolution, then filtered once with a
 * separable box kernel. The shader later takes one sample from the resulting
 * texture. `blur_type` follows the UI order: 0 none, 1 soft, 2 gaussian,
 * 3 creamy, 4 frosted.
 */
Image image_prefilter(const Image &src, uint32_t target_w, uint32_t target_h, double blur,
		      int blur_type);

} // namespace topscreen::core
