#include "ts-engine-internal.hpp"

namespace topscreen::engine_detail {

void ts_filter_ctx::sync_parent_cursor(bool hide)
{
	if (source_mode_) {
		/* The source owns the native cursor directly; no parent lease. */
		set_native_cursor(source, !hide);
		return;
	}
	obs_source_t *parent = obs_filter_get_parent(source);

	if (parent != cursor_parent_) {
		if (cursor_parent_ && cursor_acquired_)
			parent_cursor::release(cursor_parent_, cursor_hiding_);
		cursor_parent_ = parent;
		cursor_acquired_ = false;
		cursor_hiding_ = false;
		if (parent) {
			parent_cursor::acquire(parent);
			cursor_acquired_ = true;
		}
	}

	if (!cursor_acquired_ || hide == cursor_hiding_)
		return;
	parent_cursor::set_wanted(cursor_parent_, hide);
	cursor_hiding_ = hide;
}

void ts_filter_ctx::restore_parent_cursor()
{
	obs_source_t *parent = obs_filter_get_parent(source);
	if (parent && parent == cursor_parent_ && cursor_acquired_)
		parent_cursor::release(cursor_parent_, cursor_hiding_);
	cursor_parent_ = nullptr;
	cursor_acquired_ = false;
	cursor_hiding_ = false;
}

void ts_filter_ctx::start_zoom_spot(double zx0, double zy0, double zx1, double zy1, bool immediate_out,
				   double hold_override_ms)
{
	const double rw = std::max(1e-3, zx1 - zx0);
	const double rh = std::max(1e-3, zy1 - zy0);
	const bool has_rect = (rw >= cfg.highlight.min_span && rh >= cfg.highlight.min_span);
	const double base = std::max(1.0, frame.zoom.zoom);
	const core::SpotAim aim =
		core::spot_aim_for_hold(has_rect, zx0, zy0, zx1, zy1, (zx0 + zx1) * 0.5,
					(zy0 + zy1) * 0.5, base, cfg.highlight.zoom_max,
					cfg.highlight.zoom_max, false);
	if (!aim.valid)
		return;

	const bool punch = false;
	const double hold_ms = hold_override_ms >= 0.0 ? hold_override_ms
						       : cfg.highlight.zoom_hold_ms;
	zoom.spot_start_hold(aim.focus_x, aim.focus_y, aim.zoom, cfg.highlight.zoom_in_ms,
			     (punch || immediate_out) ? 0.0 : hold_ms,
			     cfg.highlight.zoom_out_ms, time_ms);
	blog(LOG_INFO, "[TopScreen] zoom spotlight: %.2fx%s", aim.zoom,
	     immediate_out ? " (quick)" : "");
	if (immediate_out)
		zoom.spot_release(time_ms);
}

void ts_filter_ctx::start_locked_highlight_zoom(double dx0, double dy0, double dx1,
						double dy1)
{
	double scene_zoom = 1.0;
	double scene_focus_x = 0.5;
	double scene_focus_y = 0.5;
	scene_view(&scene_zoom, &scene_focus_x, &scene_focus_y);
	double sx0 = 0.0, sy0 = 0.0, sx1 = 0.0, sy1 = 0.0;
	core::display_rect_to_source(dx0, dy0, dx1, dy1, scene_focus_x, scene_focus_y,
				     scene_zoom, &sx0, &sy0, &sx1, &sy1);
	sx0 = core::clamp01(sx0);
	sy0 = core::clamp01(sy0);
	sx1 = core::clamp01(sx1);
	sy1 = core::clamp01(sy1);

	const double rw = std::max(1e-3, sx1 - sx0);
	const double rh = std::max(1e-3, sy1 - sy0);
	const double fit = std::min(1.0 / rw, 1.0 / rh);
	const double target = std::max(1.0, std::min(fit, cfg.highlight.zoom_max));
	const double cx = (sx0 + sx1) * 0.5;
	const double cy = (sy0 + sy1) * 0.5;
	const double focus_x = core::center_focus_for(cx, std::max(target, 1.0001));
	const double focus_y = core::center_focus_for(cy, std::max(target, 1.0001));

	zoom.unfreeze();
	zoom.spot_start_locked(focus_x, focus_y, target, cfg.highlight.zoom_in_ms,
			       cfg.highlight.zoom_out_ms, time_ms);
	reading_dragging_ = false;
	reading_spot_x_ = focus_x;
	reading_spot_y_ = focus_y;
	hotkey_highlight_active_.store(false);
	blog(LOG_INFO, "[TopScreen] highlight locked: %.2fx rect=%.3f,%.3f..%.3f,%.3f", target,
	     sx0, sy0, sx1, sy1);
}

void ts_filter_ctx::exit_locked_highlight(bool animate)
{
	const bool had_lock = highlight.locked();
	if (had_lock) {
		if (animate)
			zoom.spot_release(time_ms);
		else
			zoom.spot_stop();
	}
	/* Also cancel an in-progress Reading drag (which freezes the view). */
	highlight.clear_lock();
	reading_dragging_ = false;
	zoom.unfreeze();
	if (had_lock) {
		hotkey_highlight_active_.store(false);
		blog(LOG_INFO, "[TopScreen] highlight unlocked");
	}
}

void ts_filter_ctx::reconfigure_models()
{
	if (!cfg.keys.enabled)
		keys_paused_.store(false);
	cursor.configure(cfg.cursor);
	zoom.configure(cfg.zoom);
	clicks.set_left(cfg.click_left);
	clicks.set_right(cfg.click_right);
	keys.configure(cfg.keys);
	highlight.configure(cfg.highlight);
	spotlight.configure(cfg.spotlight);
	spotlight.set_active(cfg.spotlight.enabled, time_ms);
	magnifier.configure(cfg.magnifier);
	magnifier.set_active(cfg.magnifier.enabled, time_ms);
	if (hotkey_highlight_active_.load() &&
	    (!cfg.effects_enabled || !cfg.highlight.hotkey_enabled)) {
		zoom.spot_release(time_ms);
		hotkey_highlight_active_.store(false);
	}
}

void ts_filter_ctx::update_input()
{
	input.poll();
	cursor.set_link(input.state().link);

	/* Keys feed the overlay even without a cursor available. */
	for (int i = 0; i < input.state().key_count; ++i)
		keys.press(input.state().key_labels[i], input.state().key_modifier[i],
				time_ms, input.state().key_modifiers[i]);
	if (input.state().key_count > 0)
		zoom.note_activity(time_ms);
}

core::ContentArea ts_filter_ctx::update_content_area()
{
	/* Same content rectangle the compositor and the shader use. The cursor
	 * tip is mapped into this area; the icon body may overhang into the
	 * padding because the shader draws it in output coordinates. */
	const double target_w = static_cast<double>(this->target_w);
	const double target_h = static_cast<double>(this->target_h);
	const bool bg_on = cfg.effects_enabled && cfg.bg.enabled;
	core::ContentArea content{};
	if (target_w > 0.0 && target_h > 0.0) {
		content = core::content_area(
			target_w, target_h, bg_on ? cfg.bg.padding_x : 0.0,
			bg_on ? cfg.bg.padding_y : 0.0, cfg.bg.keep_aspect);
		if (content.origin.x != logged_content_x_ ||
		    content.origin.y != logged_content_y_ ||
		    content.size.x != logged_content_w_ ||
		    content.size.y != logged_content_h_) {
			logged_content_x_ = content.origin.x;
			logged_content_y_ = content.origin.y;
			logged_content_w_ = content.size.x;
			logged_content_h_ = content.size.y;
			blog(LOG_INFO,
			     "[TopScreen] cursor map: target=%ux%u content=%.1fx%.1f at (%.1f,%.1f) padding=%.1fx%.1f keep_aspect=%d",
			     this->target_w, this->target_h, content.size.x, content.size.y,
			     content.origin.x, content.origin.y,
			     bg_on ? cfg.bg.padding_x : 0.0,
			     bg_on ? cfg.bg.padding_y : 0.0,
			     cfg.bg.keep_aspect ? 1 : 0);
		}
	}

	return content;
}

void ts_filter_ctx::scene_view(double *out_zoom, double *focus_x, double *focus_y) const
{
	const core::ZoomRender z = zoom.render();
	const core::SpotlightRender s = spotlight.render();
	const double amount = core::clamp01(s.amount);
	if (out_zoom)
		*out_zoom = z.zoom * std::max(1.0, s.zoom);
	if (focus_x)
		*focus_x = z.focus_x + (s.zoom_focus_x - z.focus_x) * amount;
	if (focus_y)
		*focus_y = z.focus_y + (s.zoom_focus_y - z.focus_y) * amount;
}

void ts_filter_ctx::update_pointer(const core::ContentArea &content)
{
	const double target_w = static_cast<double>(this->target_w);
	const double target_h = static_cast<double>(this->target_h);
	if (!input.available() || target_w <= 0.0 || target_h <= 0.0)
		return;

	const InputState &st = input.state();
	const core::Monitor mon{monitor_origin_.x(), monitor_origin_.y(),
				monitor_origin_.width(), monitor_origin_.height()};

	/* Combined normal + spotlight transform, then the pointer in DISPLAY
	 * (physical/composed) and SOURCE (captured pixel under it) spaces. */
	double scene_zoom = 1.0;
	double scene_focus_x = 0.5;
	double scene_focus_y = 0.5;
	scene_view(&scene_zoom, &scene_focus_x, &scene_focus_y);
	const core::PointerMapping pointer =
		core::map_pointer(st.x, st.y, mon, scene_zoom, scene_focus_x, scene_focus_y);
	const double dx = pointer.display.x;
	const double dy = pointer.display.y;
	const double sx = pointer.source.x;
	const double sy = pointer.source.y;
	pointer_display_x_ = dx;
	pointer_display_y_ = dy;
	pointer_source_x_ = sx;
	pointer_source_y_ = sy;

	if (!highlight.locked() && (st.left || st.left_pressed || st.right_pressed))
		zoom.note_activity(time_ms);

	const core::Vec2f cursor_pos =
		core::pointer_to_content_output(dx, dy, content, target_w, target_h);
	cursor.set_target(cursor_pos.x, cursor_pos.y, time_ms);
	if (!highlight.locked())
		zoom.set_cursor(sx, sy, time_ms);

	bool swallow_click = false;
	if (st.left_pressed) {
		if (highlight.locked()) {
			/* The click that dismisses a pinned highlight is consumed. */
			exit_locked_highlight(true);
			swallow_click = true;
		} else {
			if (hotkey_highlight_active_.exchange(false))
				zoom.spot_release(time_ms);
			highlight.begin(dx, dy, time_ms);
			if (core::mode_reads(cfg.highlight.mode)) {
				const core::SpotlightRender s = spotlight.render();
				reading_spot_x_ = s.zoom_focus_x;
				reading_spot_y_ = s.zoom_focus_y;
				reading_dragging_ = true;
				zoom.freeze();
			}
		}
	}
	if (st.left && !highlight.locked() && !swallow_click)
		highlight.move(dx, dy, time_ms);

	if (!st.left && prev_left && !swallow_click) {
		highlight.end(time_ms);

		double zx0 = 0.0, zy0 = 0.0, zx1 = 0.0, zy1 = 0.0;
		const bool had_request =
			highlight.take_zoom_request(&zx0, &zy0, &zx1, &zy1);
		if (had_request) {
			hotkey_highlight_active_.store(false);
			if (core::mode_reads(cfg.highlight.mode) ||
			    (cfg.highlight.lock_until_click &&
			     core::mode_wants_zoom(cfg.highlight.mode))) {
				start_locked_highlight_zoom(zx0, zy0, zx1, zy1);
			} else {
				double sx0 = 0.0, sy0 = 0.0, sx1 = 0.0, sy1 = 0.0;
				double z = 1.0, fx = 0.5, fy = 0.5;
				scene_view(&z, &fx, &fy);
				core::display_rect_to_source(zx0, zy0, zx1, zy1, fx, fy, z,
							     &sx0, &sy0, &sx1, &sy1);
				start_zoom_spot(sx0, sy0, sx1, sy1, true);
			}
		}
		reading_dragging_ = false;
		if (highlight.locked()) {
			/* A pinned mark without a spot also freezes the view. */
			if (!had_request)
				zoom.freeze();
		} else {
			zoom.unfreeze();
			/* A pinned spot stays until the next click/exit. */
			hotkey_highlight_active_.store(false);
			zoom.spot_release(time_ms);
		}
	} else if (st.left) {
		double zx0 = 0.0, zy0 = 0.0, zx1 = 0.0, zy1 = 0.0;
		if (highlight.take_zoom_request(&zx0, &zy0, &zx1, &zy1)) {
			hotkey_highlight_active_.store(false);
			double sx0 = 0.0, sy0 = 0.0, sx1 = 0.0, sy1 = 0.0;
			double z = 1.0, fx = 0.5, fy = 0.5;
			scene_view(&z, &fx, &fy);
			core::display_rect_to_source(zx0, zy0, zx1, zy1, fx, fy, z, &sx0,
						     &sy0, &sx1, &sy1);
			start_zoom_spot(sx0, sy0, sx1, sy1, false);
		}
	}

	prev_left = st.left;

	if (st.left_pressed && !swallow_click) {
		clicks.spawn(dx, dy, time_ms, false);
		if (cfg.click_sound.enabled)
			click_sound.play(false, cfg.click_sound.volume);
	}
	if (st.right_pressed) {
		clicks.spawn(dx, dy, time_ms, true);
		if (cfg.click_sound.enabled)
			click_sound.play(true, cfg.click_sound.volume);
	}
}

void ts_filter_ctx::update_active_window()
{
	if (!cfg.effects_enabled || !cfg.zoom.active_window ||
	    cfg.zoom.mode != static_cast<int>(core::ZoomMode::Smart)) {
		zoom.clear_region();
		return;
	}
	active_window_.poll(time_ms);
	const ActiveWindowRect &aw = active_window_.rect();
	if (!aw.valid || !target_w || !target_h) {
		zoom.clear_region();
		return;
	}

	const core::Monitor mon{monitor_origin_.x(), monitor_origin_.y(),
				monitor_origin_.width(), monitor_origin_.height()};
	const core::Vec2f a = core::screen_to_content(aw.x, aw.y, mon);
	const core::Vec2f b = core::screen_to_content(aw.x + aw.width, aw.y + aw.height, mon);
	const double x0 = core::clamp01(std::min(a.x, b.x));
	const double y0 = core::clamp01(std::min(a.y, b.y));
	const double x1 = core::clamp01(std::max(a.x, b.x));
	const double y1 = core::clamp01(std::max(a.y, b.y));
	if (x1 - x0 > 0.01 && y1 - y0 > 0.01) {
		if (zoom.set_region(x0, y0, x1, y1))
			zoom.note_activity(time_ms);
	} else {
		zoom.clear_region();
	}
}

} // namespace topscreen::engine_detail
