#include "core/config_limits.hpp"
#include "core/webcam.hpp"

#include "test_framework.hpp"

using namespace topscreen::core;

TEST(webcam_bubble_geometry_anchors_and_offsets)
{
	WebcamConfig cfg;
	cfg.size = 0.2;
	cfg.anchor = static_cast<int>(WebcamAnchor::BottomRight);
	cfg.offset_x = 10.0;
	cfg.offset_y = -5.0;

	const double d = webcam_bubble_diameter(cfg, 1000.0, 500.0);
	CHECK_NEAR(d, 100.0, 1e-9);
	const Vec2f c = webcam_bubble_center(cfg, 1000.0, 500.0);
	CHECK_NEAR(c.x, 960.0, 1e-9);
	CHECK_NEAR(c.y, 445.0, 1e-9);

	cfg.anchor = static_cast<int>(WebcamAnchor::TopLeft);
	cfg.offset_x = 0.0;
	cfg.offset_y = 0.0;
	const Vec2f tl = webcam_bubble_center(cfg, 1000.0, 500.0);
	CHECK_NEAR(tl.x, 50.0, 1e-9);
	CHECK_NEAR(tl.y, 50.0, 1e-9);

	cfg.anchor = static_cast<int>(WebcamAnchor::MiddleCenter);
	const Vec2f mc = webcam_bubble_center(cfg, 1000.0, 500.0);
	CHECK_NEAR(mc.x, 500.0, 1e-9);
	CHECK_NEAR(mc.y, 250.0, 1e-9);
}

TEST(webcam_bubble_sanitizes_mask_settings)
{
	WebcamConfig cfg;
	cfg.shape = 99;
	cfg.size = 0.0;
	cfg.anchor = 99;
	cfg.corner_ratio = 2.0;
	cfg.offset_x = 99999.0;
	cfg.border_width = -3.0;
	cfg.shadow_alpha = 2.0;
	cfg.shadow_blur = 999.0;
	cfg.shadow_offset_y = -999.0;

	sanitize_webcam_config(&cfg);

	CHECK_EQ(cfg.shape, static_cast<int>(WebcamShape::Rounded));
	CHECK_NEAR(cfg.size, limits::kWebcamSizeRange.min, 1e-9);
	CHECK_EQ(cfg.anchor, static_cast<int>(WebcamAnchor::BottomRight));
	CHECK_NEAR(cfg.corner_ratio, 0.5, 1e-9);
	CHECK_NEAR(cfg.offset_x, 1000.0, 1e-9);
	CHECK_NEAR(cfg.border_width, 0.0, 1e-9);
	CHECK_NEAR(cfg.shadow_alpha, 1.0, 1e-9);
	CHECK_NEAR(cfg.shadow_blur, 80.0, 1e-9);
	CHECK_NEAR(cfg.shadow_offset_y, -200.0, 1e-9);
}
