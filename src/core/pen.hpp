#pragma once

#include "types.hpp"

#include <cstdint>
#include <vector>

namespace topscreen::core {

/**
 * Incremental canvas for the freehand highlighter.
 *
 * Points arrive in content coordinates (0..1) and are stamped into an 8-bit
 * alpha mask. Stamping is incremental: only the segment added since the last
 * call is drawn, so drawing stays cheap even at 60 fps.
 *
 * The mask covers the whole content area; the renderer stretches it over the
 * content rectangle, so no per-frame texture layout is needed.
 */
class PenCanvas {
public:
	/** Prepares the canvas for a fresh stroke. */
	void reset(double aspect, int max_dim, double thickness);

	/** Appends a point (content 0..1). Safe to call with the same point. */
	void add_point(Vec2f p);

	/** Copies a whole trail (used when the model already holds the points). */
	void set_points(const Vec2f *points, int count);

	int width() const { return width_; }
	int height() const { return height_; }
	const std::vector<uint8_t> &alpha() const { return alpha_; }
	/** Changes whenever the mask changed (renderers upload on change). */
	uint32_t revision() const { return revision_; }

private:
	void stamp_segment(Vec2f a, Vec2f b);
	void stamp_disc(double cx, double cy, double radius);

	int width_ = 0;
	int height_ = 0;
	double radius_px_ = 1.0;
	std::vector<uint8_t> alpha_;
	uint32_t revision_ = 1;
	bool has_last_ = false;
	Vec2f last_{};
};

} // namespace topscreen::core
