#pragma once

#include "types.hpp"

#include <cstdint>

namespace topscreen::core {

enum class WebcamShape : int {
	Circle = 0,
	Rounded = 1,
};

/** Nine-position anchor for the bubble inside the source frame. */
enum class WebcamAnchor : int {
	TopLeft = 0,
	TopCenter,
	TopRight,
	MiddleLeft,
	MiddleCenter,
	MiddleRight,
	BottomLeft,
	BottomCenter,
	BottomRight,
};

struct WebcamConfig {
	int shape = static_cast<int>(WebcamShape::Rounded);
	double size = 0.28; // bubble diameter as a fraction of the shorter side
	int anchor = static_cast<int>(WebcamAnchor::BottomRight);
	double offset_x = 0.0; // px
	double offset_y = 0.0; // px
	double corner_ratio = 0.35; // rounded square only, fraction of the diameter
	bool mirror = true;

	bool border_enabled = true;
	uint32_t border_color = 0xFFFFFFFFu;
	double border_width = 3.0; // px

	bool shadow_enabled = true;
	uint32_t shadow_color = 0xFF000000u;
	double shadow_alpha = 0.45;
	double shadow_blur = 24.0; // px
	double shadow_offset_x = 0.0;
	double shadow_offset_y = 8.0;
};

/** Bubble center in target pixels. */
Vec2f webcam_bubble_center(const WebcamConfig &cfg, double width, double height);

/** Bubble diameter in target pixels. */
double webcam_bubble_diameter(const WebcamConfig &cfg, double width, double height);

/** Default scene-item layout for the "add webcam bubble" action. */
struct WebcamBubbleLayout {
	double diameter = 0.0;
	double margin = 0.0;
	Vec2f center;
};

/** Bottom-right bubble layout used when the action creates a camera source. */
WebcamBubbleLayout webcam_bubble_item_layout(const WebcamConfig &cfg, double width,
					     double height);

void sanitize_webcam_config(WebcamConfig *cfg);

} // namespace topscreen::core
