#include "ts-engine-internal.hpp"

namespace topscreen::engine_detail {

void ts_filter_ctx::process_actions()
{
	const uint32_t act = actions_.exchange(0);
	if (act == 0)
		return;

	if (act & A_CANCEL_HIGHLIGHT)
		exit_locked_highlight(false);

	if (act & A_TOGGLE_EFFECTS) {
		cfg.effects_enabled = !cfg.effects_enabled;
		if (!cfg.effects_enabled)
			exit_locked_highlight(false);
		push_settings();
		mark_profile_dirty();
	}
	if (act & A_SET_EFFECTS) {
		const int v = pending_effects_.exchange(-1);
		if (v >= 0) {
			cfg.effects_enabled = (v != 0);
			if (!cfg.effects_enabled)
				exit_locked_highlight(false);
			push_settings();
			mark_profile_dirty();
		}
	}
	if (act & A_TOGGLE_BG) {
		cfg.bg.enabled = !cfg.bg.enabled;
		push_settings();
		mark_profile_dirty();
	}
	if (act & (A_ZOOM_IN | A_ZOOM_OUT | A_RESET_ZOOM))
		exit_locked_highlight(false);
	if (act & A_ZOOM_IN)
		zoom.manual_step(1.25);
	if (act & A_ZOOM_OUT)
		zoom.manual_step(0.8);
	if (act & A_RESET_ZOOM)
		zoom.set_manual(1.0);
	/* Manual zoom cancels a highlighter zoom, including the hotkey toggle. */
	if (act & (A_ZOOM_IN | A_ZOOM_OUT | A_RESET_ZOOM)) {
		zoom.spot_stop();
		hotkey_highlight_active_.store(false);
	}
	if (act & A_NEXT_PRESET)
		request_preset(presets.next());
	if (act & A_PREV_PRESET)
		request_preset(presets.prev());
	if (act & A_APPLY_PRESET) {
		const int i = pending_preset_.exchange(-1);
		if (i >= 0)
			request_preset(i);
	}
	int profile_target = -1;
	if (act & A_PROFILE_SET)
		profile_target = pending_profile_.exchange(-1);
	else if (act & A_PROFILE_NEXT)
		profile_target = (current_profile_ + 1) % kProfileCount;
	else if (act & A_PROFILE_PREV)
		profile_target = (current_profile_ + kProfileCount - 1) % kProfileCount;
	if (profile_target >= 0 && profile_target < kProfileCount) {
		/* Save pending edits to the profile we are leaving. */
		flush_profile_if_dirty();
		current_profile_ = profile_target;
		last_profile = profile_target;
		apply_profile(profile_target);
		reconfigure_models();
		push_settings();
		refresh_properties();
		blog(LOG_INFO, "[TopScreen] profile activated: %s",
		     core::profile_label(profile_target));
	}
	if (act & A_TOGGLE_CLICK_SOUND) {
		cfg.click_sound.enabled = !cfg.click_sound.enabled;
		push_settings();
		mark_profile_dirty();
		blog(LOG_INFO, "[TopScreen] click sound: %s",
		     cfg.click_sound.enabled ? "on" : "off");
	}
	if (act & A_TOGGLE_HIGHLIGHT) {
		if (highlight.locked()) {
			/* Do not let the hotkey replace a pinned Reading/locked spot. */
			exit_locked_highlight(true);
		} else if (!cfg.effects_enabled || !cfg.highlight.hotkey_enabled) {
			if (hotkey_highlight_active_.load()) {
				zoom.spot_release(time_ms);
				hotkey_highlight_active_.store(false);
			}
		} else if (hotkey_highlight_active_.load()) {
			/* Second press: same release/return path as the drag zoom. */
			zoom.spot_release(time_ms);
			hotkey_highlight_active_.store(false);
			blog(LOG_INFO, "[TopScreen] hotkey highlight: release");
		} else {
			/* First press: zoom into the source point under the pointer. */
			const double cx = pointer_source_x_;
			const double cy = pointer_source_y_;
			start_zoom_spot(cx, cy, cx, cy, false, 0.0);
			hotkey_highlight_active_.store(zoom.spot_active());
			blog(LOG_INFO, "[TopScreen] hotkey highlight: %s",
			     hotkey_highlight_active_.load() ? "active" : "no-op");
		}
	}
	if (act & A_TOGGLE_SPOTLIGHT) {
		exit_locked_highlight(false);
		cfg.spotlight.enabled = !cfg.spotlight.enabled;
		if (cfg.spotlight.enabled) {
			cfg.magnifier.enabled = false;
			magnifier.set_active(false, time_ms);
		}
		spotlight.set_active(cfg.spotlight.enabled, time_ms);
		push_settings();
		mark_profile_dirty();
		blog(LOG_INFO, "[TopScreen] spotlight: %s", cfg.spotlight.enabled ? "on" : "off");
	}
	if (act & A_TOGGLE_MAGNIFIER) {
		exit_locked_highlight(false);
		cfg.magnifier.enabled = !cfg.magnifier.enabled;
		if (cfg.magnifier.enabled) {
			cfg.spotlight.enabled = false;
			spotlight.set_active(false, time_ms);
		}
		magnifier.set_active(cfg.magnifier.enabled, time_ms);
		push_settings();
		mark_profile_dirty();
		blog(LOG_INFO, "[TopScreen] magnifier: %s", cfg.magnifier.enabled ? "on" : "off");
	}
	if (act & A_REVERT_PROFILE) {
		if (profile_baseline_valid_) {
			exit_locked_highlight(false);
			cfg = profile_baseline_;
			reconfigure_models();
			click_sound.load(cfg.click_sound.file_left.c_str(),
					 cfg.click_sound.file_right.c_str());
			push_settings();
			refresh_properties();
			profile_dirty_ = false;
			profile_edited_.store(false);
			blog(LOG_INFO, "[TopScreen] profile reverted to activation state: %s",
			     core::profile_label(current_profile_));
		}
	}
	if (act & A_DUPLICATE_PROFILE) {
		const int to = pending_duplicate_.exchange(-1);
		if (to >= 0 && to < kProfileCount && to != current_profile_) {
			flush_profile_if_dirty();
			obs_data_t *s = obs_source_get_settings(source);
			const core::Config copy = profile_store::load_slot(s, current_profile_);
			profile_store::store_slot(s, to, copy);
			std::string name = profile_store::load_name(s, current_profile_);
			if (name.empty())
				name = core::profile_label(current_profile_);
			profile_store::store_name(s, to, name + " (copy)");
			obs_source_update(source, s);
			obs_data_release(s);
			refresh_properties();
			blog(LOG_INFO, "[TopScreen] profile duplicated: %s -> %s",
			     core::profile_label(current_profile_), core::profile_label(to));
		}
	}
	if (act & A_TOGGLE_KEYS) {
		keys_paused_.store(!keys_paused_.load());
		blog(LOG_INFO, "[TopScreen] keystrokes paused: %s",
		     keys_paused_.load() ? "yes" : "no");
	}
	if (act & A_SAVE_LOOK) {
		std::string name;
		{
			std::lock_guard<std::mutex> lock(look_mutex_);
			name.swap(pending_look_name_);
		}
		std::string error;
		const int index = presets.save_user(name, cfg, &error);
		if (index >= 0) {
			preset_cache_.store(index);
			blog(LOG_INFO, "[TopScreen] look saved: %s", name.c_str());
		} else {
			blog(LOG_WARNING, "[TopScreen] could not save look: %s", error.c_str());
		}
	}
	if (act & A_TEST_CLICK) {
		const int side = pending_click_test_.exchange(-1);
		if (side >= 0)
			click_sound.play(side == 1, cfg.click_sound.volume);
	}
	if (act & A_DELETE_LOOK) {
		const int index = pending_look_delete_.exchange(-1);
		std::string error;
		if (presets.delete_user(index, &error)) {
			preset_cache_.store(presets.current());
			blog(LOG_INFO, "[TopScreen] look deleted");
		} else {
			blog(LOG_WARNING, "[TopScreen] could not delete look: %s", error.c_str());
		}
	}
	if (act & A_RESET_PROFILES) {
		obs_data_t *s = obs_source_get_settings(source);
		profile_store::reset_slots(s, kProfileSeedRevision);
		obs_source_update(source, s);
		obs_data_release(s);
		profile_dirty_ = false;
		apply_profile(current_profile_);
		reconfigure_models();
		push_settings();
		refresh_properties();
		blog(LOG_INFO, "[TopScreen] profiles restored to defaults (A..J)");
	}
	if (act & A_SAVE_PROFILE)
		do_save_profile();
	if (act & (A_EXPORT_PROFILES | A_IMPORT_PROFILES)) {
		std::string path;
		std::string text;
		std::string scenes;
		{
			std::lock_guard<std::mutex> lock(profile_io_mutex_);
			path.swap(profile_io_path_);
			text.swap(profile_io_text_);
			scenes.swap(profile_io_scenes_);
		}
		if (act & A_EXPORT_PROFILES)
			do_export_profiles(path, scenes);
		if (act & A_IMPORT_PROFILES)
			do_import_profiles(text);
	}
}

} // namespace topscreen::engine_detail
