#include "webcam.hpp"

#include "config_limits.hpp"

#include <algorithm>
#include <cmath>

namespace topscreen::core {

namespace {

double bounded(double value, double lo, double hi, double fallback)
{
	if (!std::isfinite(value))
		value = fallback;
	return std::clamp(value, lo, hi);
}

Vec2f anchor_position(int anchor)
{
	switch (static_cast<WebcamAnchor>(anchor)) {
	case WebcamAnchor::TopLeft:
		return {0.0, 0.0};
	case WebcamAnchor::TopCenter:
		return {0.5, 0.0};
	case WebcamAnchor::TopRight:
		return {1.0, 0.0};
	case WebcamAnchor::MiddleLeft:
		return {0.0, 0.5};
	case WebcamAnchor::MiddleCenter:
		return {0.5, 0.5};
	case WebcamAnchor::MiddleRight:
		return {1.0, 0.5};
	case WebcamAnchor::BottomLeft:
		return {0.0, 1.0};
	case WebcamAnchor::BottomCenter:
		return {0.5, 1.0};
	case WebcamAnchor::BottomRight:
	default:
		return {1.0, 1.0};
	}
}

} // namespace

double webcam_bubble_diameter(const WebcamConfig &cfg, double width, double height)
{
	const double shorter = std::max(1.0, std::min(width, height));
	return std::clamp(cfg.size, 0.1, 1.0) * shorter;
}

Vec2f webcam_bubble_center(const WebcamConfig &cfg, double width, double height)
{
	const double diameter = webcam_bubble_diameter(cfg, width, height);
	const Vec2f anchor = anchor_position(cfg.anchor);
	const double radius = diameter * 0.5;
	return {radius + anchor.x * std::max(0.0, width - diameter) + cfg.offset_x,
		radius + anchor.y * std::max(0.0, height - diameter) + cfg.offset_y};
}

void sanitize_webcam_config(WebcamConfig *cfg)
{
	if (!cfg)
		return;
	cfg->shape = std::clamp(cfg->shape, static_cast<int>(WebcamShape::Circle),
				static_cast<int>(WebcamShape::Rounded));
	cfg->size = bounded(cfg->size, limits::kWebcamSizeRange.min,
			    limits::kWebcamSizeRange.max, 0.28);
	cfg->anchor = std::clamp(cfg->anchor, static_cast<int>(WebcamAnchor::TopLeft),
				 static_cast<int>(WebcamAnchor::BottomRight));
	cfg->offset_x = bounded(cfg->offset_x, limits::kWebcamOffsetRange.min,
				limits::kWebcamOffsetRange.max, 0.0);
	cfg->offset_y = bounded(cfg->offset_y, limits::kWebcamOffsetRange.min,
				limits::kWebcamOffsetRange.max, 0.0);
	cfg->corner_ratio = bounded(cfg->corner_ratio, limits::kWebcamCornerRange.min,
				    limits::kWebcamCornerRange.max, 0.35);
	cfg->border_width = bounded(cfg->border_width, limits::kWebcamBorderWidthRange.min,
				    limits::kWebcamBorderWidthRange.max, 3.0);
	cfg->shadow_alpha = bounded(cfg->shadow_alpha, limits::kUnitRange.min,
				    limits::kUnitRange.max, 0.45);
	cfg->shadow_blur = bounded(cfg->shadow_blur, limits::kWebcamShadowBlurRange.min,
				   limits::kWebcamShadowBlurRange.max, 24.0);
	cfg->shadow_offset_x = bounded(cfg->shadow_offset_x,
				       limits::kWebcamShadowOffsetRange.min,
				       limits::kWebcamShadowOffsetRange.max, 0.0);
	cfg->shadow_offset_y = bounded(cfg->shadow_offset_y,
				       limits::kWebcamShadowOffsetRange.min,
				       limits::kWebcamShadowOffsetRange.max, 8.0);
}

} // namespace topscreen::core
