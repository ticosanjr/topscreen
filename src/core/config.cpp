#include "config.hpp"

#include <algorithm>
#include <cmath>

#include "cursor_catalog.hpp"
#include "easing.hpp"
#include "highlighter.hpp"
#include "types.hpp"

namespace topscreen::core {

namespace {

double bounded(double value, double lo, double hi, double fallback)
{
	if (!std::isfinite(value))
		value = fallback;
	return std::clamp(value, lo, hi);
}

int bounded_int(int value, int lo, int hi)
{
	return std::clamp(value, lo, hi);
}

/* Keeps -45° and 315° equivalent while still rejecting non-finite input. */
double wrapped_angle(double value, double fallback)
{
	if (!std::isfinite(value))
		value = fallback;
	value = std::fmod(value, 360.0);
	if (value < 0.0)
		value += 360.0;
	return value;
}

void sanitize_click(ClickConfig *c, const ClickConfig &def)
{
	c->effect = bounded_int(c->effect, static_cast<int>(ClickEffect::Ripple),
				static_cast<int>(ClickEffect::Flare));
	c->size = bounded(c->size, 0.1, 4.0, def.size);
	c->duration_ms = bounded(c->duration_ms, 100.0, 2000.0, def.duration_ms);
	c->easing = bounded_int(c->easing, EASE_LINEAR, EASE_IN_OUT_CUBIC);
}

} // namespace

void sanitize_config(Config *cfg)
{
	if (!cfg)
		return;

	const Config def;

	/* Background */
	cfg->bg.mode = bounded_int(cfg->bg.mode, static_cast<int>(BgMode::Solid),
				   static_cast<int>(BgMode::Wallpaper));
	cfg->bg.gradient_angle = wrapped_angle(cfg->bg.gradient_angle, def.bg.gradient_angle);
	cfg->bg.gradient_center_x =
		bounded(cfg->bg.gradient_center_x, 0.0, 1.0, def.bg.gradient_center_x);
	cfg->bg.gradient_center_y =
		bounded(cfg->bg.gradient_center_y, 0.0, 1.0, def.bg.gradient_center_y);
	cfg->bg.image_blur = bounded(cfg->bg.image_blur, 0.0, 60.0, def.bg.image_blur);
	cfg->bg.image_blur_type = bounded_int(cfg->bg.image_blur_type, 0, 4);
	cfg->bg.padding_x = bounded(cfg->bg.padding_x, 0.0, 400.0, def.bg.padding_x);
	cfg->bg.padding_y = bounded(cfg->bg.padding_y, 0.0, 400.0, def.bg.padding_y);
	cfg->bg.corner_radius =
		bounded(cfg->bg.corner_radius, 0.0, 200.0, def.bg.corner_radius);
	cfg->bg.shadow_alpha = bounded(cfg->bg.shadow_alpha, 0.0, 1.0, def.bg.shadow_alpha);
	cfg->bg.shadow_blur = bounded(cfg->bg.shadow_blur, 0.0, 200.0, def.bg.shadow_blur);
	cfg->bg.shadow_offset_x =
		bounded(cfg->bg.shadow_offset_x, -100.0, 100.0, def.bg.shadow_offset_x);
	cfg->bg.shadow_offset =
		bounded(cfg->bg.shadow_offset, -100.0, 100.0, def.bg.shadow_offset);

	/* Cursor */
	cfg->cursor.style = cursor_style_clamp(cfg->cursor.style);
	cfg->cursor.size = bounded(cfg->cursor.size, 0.5, 4.0, def.cursor.size);
	cfg->cursor.smooth = bounded(cfg->cursor.smooth, 0.0, 1.0, def.cursor.smooth);
	cfg->cursor.sway = bounded(cfg->cursor.sway, 0.0, 1.0, def.cursor.sway);
	cfg->cursor.idle_delay_ms =
		bounded(cfg->cursor.idle_delay_ms, 0.0, 5000.0, def.cursor.idle_delay_ms);
	cfg->cursor.loop_ms = bounded(cfg->cursor.loop_ms, 1000.0, 20000.0, def.cursor.loop_ms);

	/* Zoom */
	cfg->zoom.mode = bounded_int(cfg->zoom.mode, static_cast<int>(ZoomMode::Off),
				     static_cast<int>(ZoomMode::Manual));
	cfg->zoom.amount = bounded(cfg->zoom.amount, 1.0, 4.0, def.zoom.amount);
	cfg->zoom.duration_ms = bounded(cfg->zoom.duration_ms, 100.0, 2000.0, def.zoom.duration_ms);
	cfg->zoom.idle_timeout_ms =
		bounded(cfg->zoom.idle_timeout_ms, 200.0, 10000.0, def.zoom.idle_timeout_ms);
	cfg->zoom.pan_amount = bounded(cfg->zoom.pan_amount, 0.0, 1.0, def.zoom.pan_amount);
	cfg->zoom.center_amount = bounded(cfg->zoom.center_amount, 0.0, 1.0, def.zoom.center_amount);
	cfg->zoom.easing = bounded_int(cfg->zoom.easing, EASE_LINEAR, EASE_IN_OUT_CUBIC);

	/* Clicks */
	sanitize_click(&cfg->click_left, def.click_left);
	sanitize_click(&cfg->click_right, def.click_right);
	cfg->click_sound.volume =
		bounded(cfg->click_sound.volume, 0.0, 1.0, def.click_sound.volume);

	/* Motion blur */
	cfg->motion_blur.amount =
		bounded(cfg->motion_blur.amount, 0.0, 1.0, def.motion_blur.amount);

	/* Spotlight */
	cfg->spotlight.shape = bounded_int(cfg->spotlight.shape,
					   static_cast<int>(SpotlightShape::Circle),
					   static_cast<int>(SpotlightShape::Square));
	cfg->spotlight.radius = bounded(cfg->spotlight.radius, 0.05, 1.0, def.spotlight.radius);
	cfg->spotlight.corner_radius =
		bounded(cfg->spotlight.corner_radius, 0.0, 200.0, def.spotlight.corner_radius);
	cfg->spotlight.dim = bounded(cfg->spotlight.dim, 0.0, 1.0, def.spotlight.dim);
	cfg->spotlight.border_width =
		bounded(cfg->spotlight.border_width, 0.5, 40.0, def.spotlight.border_width);
	cfg->spotlight.shadow_alpha =
		bounded(cfg->spotlight.shadow_alpha, 0.0, 1.0, def.spotlight.shadow_alpha);
	cfg->spotlight.shadow_blur =
		bounded(cfg->spotlight.shadow_blur, 0.0, 200.0, def.spotlight.shadow_blur);
	cfg->spotlight.shadow_offset_x =
		bounded(cfg->spotlight.shadow_offset_x, -100.0, 100.0,
			def.spotlight.shadow_offset_x);
	cfg->spotlight.shadow_offset_y =
		bounded(cfg->spotlight.shadow_offset_y, -100.0, 100.0,
			def.spotlight.shadow_offset_y);
	cfg->spotlight.blur_amount =
		bounded(cfg->spotlight.blur_amount, 0.0, 60.0, def.spotlight.blur_amount);
	cfg->spotlight.zoom_amount =
		bounded(cfg->spotlight.zoom_amount, 1.0, 4.0, def.spotlight.zoom_amount);
	cfg->spotlight.in_ms = bounded(cfg->spotlight.in_ms, 0.0, 2000.0, def.spotlight.in_ms);
	cfg->spotlight.out_ms = bounded(cfg->spotlight.out_ms, 0.0, 2000.0, def.spotlight.out_ms);
	cfg->spotlight.drop_px = bounded(cfg->spotlight.drop_px, 0.0, 200.0, def.spotlight.drop_px);

	/* Key overlay */
	cfg->keys.ttl_ms = bounded(cfg->keys.ttl_ms, 500.0, 8000.0, def.keys.ttl_ms);
	cfg->keys.max_keys = bounded_int(cfg->keys.max_keys, 1, 12);
	cfg->keys.pos_x = bounded(cfg->keys.pos_x, 0.0, 1.0, def.keys.pos_x);
	cfg->keys.pos_y = bounded(cfg->keys.pos_y, 0.0, 1.0, def.keys.pos_y);
	cfg->keys.size = bounded(cfg->keys.size, 0.4, 4.0, def.keys.size);

	/* Highlighter */
	cfg->highlight.mode = bounded_int(cfg->highlight.mode,
					  static_cast<int>(HighlightMode::Box),
					  static_cast<int>(HighlightMode::PressZoom));
	cfg->highlight.hold_ms = bounded(cfg->highlight.hold_ms, 0.0, 5000.0,
					 def.highlight.hold_ms);
	cfg->highlight.fade_ms = bounded(cfg->highlight.fade_ms, 0.0, 2000.0,
					 def.highlight.fade_ms);
	cfg->highlight.opacity = bounded(cfg->highlight.opacity, 0.0, 1.0,
					 def.highlight.opacity);
	cfg->highlight.thickness = bounded(cfg->highlight.thickness, 0.001, 0.05,
					   def.highlight.thickness);
	cfg->highlight.zoom_dir = bounded_int(cfg->highlight.zoom_dir, 0, 3);
	cfg->highlight.zoom_delay_ms = bounded(cfg->highlight.zoom_delay_ms, 0.0, 5000.0,
					       def.highlight.zoom_delay_ms);
	cfg->highlight.zoom_max = bounded(cfg->highlight.zoom_max, 1.2, 8.0,
					  def.highlight.zoom_max);
	cfg->highlight.zoom_in_ms = bounded(cfg->highlight.zoom_in_ms, 50.0, 2000.0,
					    def.highlight.zoom_in_ms);
	cfg->highlight.zoom_hold_ms = bounded(cfg->highlight.zoom_hold_ms, 0.0, 5000.0,
					      def.highlight.zoom_hold_ms);
	cfg->highlight.zoom_out_ms = bounded(cfg->highlight.zoom_out_ms, 100.0, 8000.0,
					     def.highlight.zoom_out_ms);
	cfg->highlight.press_delay_ms = bounded(cfg->highlight.press_delay_ms, 0.0, 3000.0,
						def.highlight.press_delay_ms);
}

} // namespace topscreen::core
