#include "image_filter.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace topscreen::core {

namespace {

int clamp_index(int v, int n)
{
	return v < 0 ? 0 : (v >= n ? n - 1 : v);
}

/* Separable box blur with a sliding window, O(pixels) per pass. */
void box_blur_pass(const uint8_t *src, uint8_t *dst, int w, int h, int radius, bool horizontal)
{
	const int div = radius * 2 + 1;
	if (horizontal) {
		for (int y = 0; y < h; ++y) {
			int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
			for (int k = -radius; k <= radius; ++k) {
				const int x = clamp_index(k, w);
				const int i = (y * w + x) * 4;
				s0 += src[i];
				s1 += src[i + 1];
				s2 += src[i + 2];
				s3 += src[i + 3];
			}
			for (int x = 0; x < w; ++x) {
				const int o = (y * w + x) * 4;
				dst[o] = static_cast<uint8_t>(s0 / div);
				dst[o + 1] = static_cast<uint8_t>(s1 / div);
				dst[o + 2] = static_cast<uint8_t>(s2 / div);
				dst[o + 3] = static_cast<uint8_t>(s3 / div);

				const int xo = clamp_index(x - radius, w);
				const int xi = clamp_index(x + radius + 1, w);
				const int io = (y * w + xo) * 4;
				const int ii = (y * w + xi) * 4;
				s0 += src[ii] - src[io];
				s1 += src[ii + 1] - src[io + 1];
				s2 += src[ii + 2] - src[io + 2];
				s3 += src[ii + 3] - src[io + 3];
			}
		}
		return;
	}

	for (int x = 0; x < w; ++x) {
		int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
		for (int k = -radius; k <= radius; ++k) {
			const int y = clamp_index(k, h);
			const int i = (y * w + x) * 4;
			s0 += src[i];
			s1 += src[i + 1];
			s2 += src[i + 2];
			s3 += src[i + 3];
		}
		for (int y = 0; y < h; ++y) {
			const int o = (y * w + x) * 4;
			dst[o] = static_cast<uint8_t>(s0 / div);
			dst[o + 1] = static_cast<uint8_t>(s1 / div);
			dst[o + 2] = static_cast<uint8_t>(s2 / div);
			dst[o + 3] = static_cast<uint8_t>(s3 / div);

			const int yo = clamp_index(y - radius, h);
			const int yi = clamp_index(y + radius + 1, h);
			const int io = (yo * w + x) * 4;
			const int ii = (yi * w + x) * 4;
			s0 += src[ii] - src[io];
			s1 += src[ii + 1] - src[io + 1];
			s2 += src[ii + 2] - src[io + 2];
			s3 += src[ii + 3] - src[io + 3];
		}
	}
}

} // namespace

Image image_resize_rgba(const Image &src, int dst_width, int dst_height)
{
	Image out;
	if (src.empty() || dst_width <= 0 || dst_height <= 0)
		return out;
	out.width = dst_width;
	out.height = dst_height;
	out.pixels.resize(static_cast<size_t>(dst_width) * static_cast<size_t>(dst_height) * 4u);
	if (dst_width == src.width && dst_height == src.height) {
		out.pixels = src.pixels;
		return out;
	}

	for (int y = 0; y < dst_height; ++y) {
		const float sy = (static_cast<float>(y) + 0.5f) *
				 static_cast<float>(src.height) / static_cast<float>(dst_height) -
			 0.5f;
		const int y0 = static_cast<int>(std::floor(sy));
		const int y1 = clamp_index(y0 + 1, src.height);
		const float fy = sy - static_cast<float>(y0);
		for (int x = 0; x < dst_width; ++x) {
			const float sx = (static_cast<float>(x) + 0.5f) *
					 static_cast<float>(src.width) / static_cast<float>(dst_width) -
				 0.5f;
			const int x0 = clamp_index(static_cast<int>(std::floor(sx)), src.width);
			const int x1 = clamp_index(x0 + 1, src.width);
			const float fx = sx - static_cast<float>(static_cast<int>(std::floor(sx)));
			const int i00 = (clamp_index(y0, src.height) * src.width + x0) * 4;
			const int i10 = (clamp_index(y0, src.height) * src.width + x1) * 4;
			const int i01 = (y1 * src.width + x0) * 4;
			const int i11 = (y1 * src.width + x1) * 4;
			const int o = (y * dst_width + x) * 4;
			for (int c = 0; c < 4; ++c) {
				const float a = src.pixels[i00 + c] * (1.0f - fx) +
						src.pixels[i10 + c] * fx;
				const float b = src.pixels[i01 + c] * (1.0f - fx) +
						src.pixels[i11 + c] * fx;
				out.pixels[o + c] =
					static_cast<uint8_t>(a * (1.0f - fy) + b * fy + 0.5f);
			}
		}
	}
	return out;
}

Image image_limit_max(const Image &src, int max_dim)
{
	if (src.empty() || max_dim <= 0)
		return src;
	const int longest = std::max(src.width, src.height);
	if (longest <= max_dim)
		return src;
	const float scale = static_cast<float>(max_dim) / static_cast<float>(longest);
	const int w = std::max(1, static_cast<int>(std::lround(src.width * scale)));
	const int h = std::max(1, static_cast<int>(std::lround(src.height * scale)));
	return image_resize_rgba(src, w, h);
}

Image image_prefilter(const Image &src, uint32_t target_w, uint32_t target_h, double blur,
		      int blur_type)
{
	if (src.empty() || blur <= 0.01 || blur_type <= 0)
		return src;

	const float spread = blur_type < 1.5 ? 0.70f
			     : (blur_type >= 3.5 ? 0.66f
						 : (blur_type >= 2.5 ? 0.62f : 0.50f));
	int bw = src.width;
	int bh = src.height;
	if (blur >= 4.0 && bw > 960) {
		bw = std::max(1, bw / 2);
		bh = std::max(1, bh / 2);
	}
	Image out = image_resize_rgba(src, bw, bh);

	const float scale =
		std::max(static_cast<float>(bw) / static_cast<float>(std::max(target_w, 1u)),
			 static_cast<float>(bh) / static_cast<float>(std::max(target_h, 1u)));
	const int radius = static_cast<int>(std::lround(blur * spread * scale));
	if (radius <= 0)
		return out;

	std::vector<uint8_t> tmp(out.pixels.size());
	const bool smooth = blur_type >= 2 && radius >= 2;
	const int passes = smooth ? 2 : 1;
	const int pass_radius = smooth ? std::max(1, radius / 2) : radius;
	for (int pass = 0; pass < passes; ++pass) {
		box_blur_pass(out.pixels.data(), tmp.data(), bw, bh, pass_radius, true);
		box_blur_pass(tmp.data(), out.pixels.data(), bw, bh, pass_radius, false);
	}
	return out;
}

} // namespace topscreen::core
