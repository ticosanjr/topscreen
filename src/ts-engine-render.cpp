#include "ts-engine-internal.hpp"

namespace topscreen::engine_detail {

void ts_filter_ctx::update_visual_state(const core::ContentArea &content)
{
	UNUSED_PARAMETER(content);
	update_active_window();
	const double target_w = static_cast<double>(this->target_w);
	const double target_h = static_cast<double>(this->target_h);
	clicks.tick(time_ms);
	cursor.update(time_ms);
	zoom.update(time_ms);
	keys.tick(time_ms);
	highlight.update(time_ms);

	frame.cursor = cursor.render();
	if (!cursor_hotkey_visible_)
		frame.cursor.visible = false;

	/* The spotlight mask is drawn in OUTPUT coordinates (the smoothed cursor
	 * overlay); its ZOOM focus is the SOURCE point under the physical pointer.
	 * A pinned highlight owns the zoom focus: moving the mouse must not move
	 * the scene through an active spotlight. */
	const bool freeze_spot = reading_dragging_ || highlight.locked();
	const double spot_zoom_x = freeze_spot ? reading_spot_x_ : pointer_source_x_;
	const double spot_zoom_y = freeze_spot ? reading_spot_y_ : pointer_source_y_;
	spotlight.update(time_ms, frame.cursor.x, frame.cursor.y, spot_zoom_x, spot_zoom_y);
	frame.spotlight = spotlight.render();
	magnifier.update(time_ms, frame.cursor.x, frame.cursor.y,
			      target_w > 0.0 && target_h > 0.0 ? target_w / target_h : 1.0);
	frame.magnifier = magnifier.render();
	frame.zoom = zoom.render();
	frame.click_count = clicks.collect(time_ms, frame.clicks, kShaderClickSlots);
	frame.key_count =
		keys.collect(time_ms, frame.keys, FrameState::kMaxKeys);
	if (keys_paused_.load())
		frame.key_count = 0;
	frame.highlight = highlight.render();

	/* Publishes state to the dock (UI thread). */
	effects_cache_.store(cfg.effects_enabled);
	keys_enabled_cache_.store(cfg.keys.enabled);
	preset_cache_.store(presets.current());
	profile_cache_.store(current_profile_);
	input_tier_cache_.store(input.tier());
}

void ts_filter_ctx::render_filter(gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	if (!loaded) {
		obs_source_skip_video_filter(source);
		return;
	}

	/* The target source is only guaranteed valid inside the render callbacks. */
	obs_source_t *target = obs_filter_get_target(source);
	if (target) {
		target_w = obs_source_get_base_width(target);
		target_h = obs_source_get_base_height(target);
	}

	/* The capture's native cursor follows the master switch and the TopScreen
	 * cursor setting: hidden only while TopScreen actually draws its own. */
	sync_parent_cursor(cfg.effects_enabled && cfg.cursor.enabled);

	compositor.render(source, target, target_w, target_h, cfg, frame);
}

void ts_filter_ctx::render_texture(gs_texture_t *texture, uint32_t width, uint32_t height,
				   gs_effect_t *effect)
{
	if (!loaded || !texture || !width || !height)
		return;
	target_w = width;
	target_h = height;
	sync_parent_cursor(cfg.effects_enabled && cfg.cursor.enabled);
	compositor.render_texture(texture, width, height, cfg, frame, effect);
}

} // namespace topscreen::engine_detail
