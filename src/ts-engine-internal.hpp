#pragma once

#include "ts-engine.hpp"
#include "capture/ts-capture-source.hpp"

#include "audio/ts-click-sound.hpp"
#include "core/coord.hpp"
#include "core/file.hpp"
#include "core/keys.hpp"
#include "core/math.hpp"
#include "core/geometry.hpp"
#include "core/mapping.hpp"
#include "core/profiles.hpp"
#include "core/profiles_io.hpp"
#include "core/magnifier.hpp"
#include "core/spotlight.hpp"
#include "hotkeys/ts-hotkeys.hpp"
#include "input/ts-input.hpp"
#include "input/ts-active-window.hpp"
#include "ts-monitor-origin.hpp"
#include "render/ts-compositor.hpp"
#include "render/ts-frame.hpp"
#include "ts-parent-cursor.hpp"
#include "ts-presets.hpp"
#include "ts-profile-bundle.hpp"
#include "ts-profile-store.hpp"
#include "ts-profiles-ui.hpp"
#include "ts-properties.hpp"
#include "ts-registry.hpp"
#include "ts-scene-profiles.hpp"
#include "ts-settings.hpp"
#include "ts-version.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>

namespace topscreen::engine_detail {

using namespace topscreen;
using namespace topscreen::core::keys;


/* Actions queued by hotkeys (hotkey thread) and by the dock (UI thread).
 * They are consumed in video_tick, guaranteeing single-thread access to the models. */
enum ActionBits : uint32_t {
	A_TOGGLE_EFFECTS = 1u << 0,
	A_SET_EFFECTS = 1u << 1,
	A_TOGGLE_BG = 1u << 2,
	A_ZOOM_IN = 1u << 3,
	A_ZOOM_OUT = 1u << 4,
	A_RESET_ZOOM = 1u << 5,
	A_NEXT_PRESET = 1u << 6,
	A_PREV_PRESET = 1u << 7,
	A_APPLY_PRESET = 1u << 8,
	A_PROFILE_SET = 1u << 9,
	A_SAVE_PROFILE = 1u << 10,
	A_EXPORT_PROFILES = 1u << 11,
	A_IMPORT_PROFILES = 1u << 12,
	A_TOGGLE_HIGHLIGHT = 1u << 13,
	A_TOGGLE_SPOTLIGHT = 1u << 14,
	A_TOGGLE_MAGNIFIER = 1u << 15,
	A_TOGGLE_CURSOR = 1u << 16,
	A_RESET_PROFILES = 1u << 17,
	A_PROFILE_NEXT = 1u << 18,
	A_PROFILE_PREV = 1u << 19,
	A_TOGGLE_CLICK_SOUND = 1u << 20,
	A_CANCEL_HIGHLIGHT = 1u << 21,
	A_REVERT_PROFILE = 1u << 22,
	A_DUPLICATE_PROFILE = 1u << 23,
	A_TOGGLE_KEYS = 1u << 24,
	A_SAVE_LOOK = 1u << 25,
	A_DELETE_LOOK = 1u << 26,
	A_TEST_CLICK = 1u << 27,
};


/**
 * TopScreen filter instance.
 *
 * Concurrency:
 *  - cfg/models: only the video thread (tick/render) after create.
 *  - hotkeys/UI: queue actions (atomic); the tick applies them.
 *  - update(): only flags "reload"; the tick reloads from obs_data.
 */
struct ts_filter_ctx final : public HotkeyContext, public FilterControl {
	obs_source_t *source = nullptr;
	bool source_mode_ = false; // true for TopScreen Capture, false for the filter
	std::string source_name_; // cached OBS source name (dock target selector)
	core::Config cfg;
	Compositor compositor;
	Input input;
	Hotkeys hotkeys;
	PresetStore presets;
	ClickSound click_sound;

	core::CursorModel cursor;
	core::ZoomModel zoom;
	core::ClickFxModel clicks;
	core::KeyOverlayModel keys;
	core::HighlighterModel highlight;
	core::SpotlightModel spotlight;
	core::MagnifierModel magnifier;
	MonitorOrigin monitor_origin_;
	ActiveWindow active_window_;
	FrameState frame;

	uint32_t target_w = 0;
	uint32_t target_h = 0;
	double time_ms = 0.0;
	bool loaded = false;
	obs_source_t *cursor_parent_ = nullptr; // parent that owns the cursor lease
	bool cursor_acquired_ = false;
	bool cursor_hiding_ = false;
	bool prev_left = false;
	bool cursor_hotkey_visible_ = true;
	/* Pointer in the two coordinate spaces (see update_pointer). */
	double pointer_display_x_ = 0.5;
	double pointer_display_y_ = 0.5;
	double pointer_source_x_ = 0.5;
	double pointer_source_y_ = 0.5;
	/* Spotlight zoom focus is frozen while a Reading selection is dragged or
	 * pinned, so moving the mouse cannot move the scene through it. */
	bool reading_dragging_ = false;
	double reading_spot_x_ = 0.5;
	double reading_spot_y_ = 0.5;
	std::atomic<bool> hotkey_highlight_active_{false};

	int current_profile_ = 0;
	int last_profile = -1;
	/* Auto-save debounce: a slider drag must not write the profile slot on
	 * every tick. Flushed on profile switch/save/export/destroy. */
	bool profile_dirty_ = false;
	/* Fingerprint of the last settings we pushed. A reload that loads the
	 * same values is self-induced and must not turn into a profile edit. */
	std::string pushed_fingerprint_;
	/* Snapshot taken when a profile is activated; Revert restores it. */
	core::Config profile_baseline_;
	std::atomic<bool> profile_baseline_valid_{false};
	std::atomic<bool> profile_edited_{false};
	std::atomic<bool> keys_paused_{false};
	std::atomic<bool> keys_enabled_cache_{true};
	double profile_dirty_since_ms_ = 0.0;
	static constexpr double kProfileSaveDebounceMs = 400.0;

	/* Last content rectangle logged, so the cursor mapping can be diagnosed
	 * from the OBS log without spamming a line every frame. */
	double logged_content_x_ = -1.0;
	double logged_content_y_ = -1.0;
	double logged_content_w_ = -1.0;
	double logged_content_h_ = -1.0;

	std::atomic<uint32_t> actions_{0};
	std::atomic<int> pending_effects_{-1};
	std::atomic<int> pending_preset_{-1};
	std::atomic<int> pending_profile_{-1};
	std::atomic<int> pending_duplicate_{-1};
	std::atomic<int> pending_click_test_{-1};
	std::atomic<bool> pending_reload_{false};
	std::atomic<bool> effects_cache_{true};
	std::atomic<int> preset_cache_{-1};
	std::atomic<int> profile_cache_{0};
	std::atomic<int> input_tier_cache_{0};

	/* User look requests (UI thread -> video thread). */
	std::mutex look_mutex_;
	std::string pending_look_name_;
	std::atomic<int> pending_look_delete_{-1};

	/* Profiles backup requests (UI thread -> video thread). */
	std::mutex profile_io_mutex_;
	std::string profile_io_path_;
	std::string profile_io_text_;
	std::string profile_io_scenes_;
	std::atomic<int> profile_io_result_{0};

	void push_settings();
	void refresh_properties();
	void reconfigure_models();
	void request_preset(int index);
	void apply_profile(int index);
	void store_profile_slot(int index);
	void mark_profile_dirty();
	void flush_profile_if_dirty();
	void do_save_profile();
	void do_export_profiles(const std::string &path, const std::string &scenes);
	void do_import_profiles(const std::string &json_text);
	void process_actions();
	void action_cancel_highlight(uint32_t act);
	void action_effects(uint32_t act);
	void action_zoom(uint32_t act);
	void action_presets(uint32_t act);
	void action_profile_switch(uint32_t act);
	void action_click_sound(uint32_t act);
	void action_highlight(uint32_t act);
	void action_spotlight(uint32_t act);
	void action_magnifier(uint32_t act);
	void action_revert_profile(uint32_t act);
	void action_duplicate_profile(uint32_t act);
	void action_keys(uint32_t act);
	void action_save_look(uint32_t act);
	void action_test_click(uint32_t act);
	void action_delete_look(uint32_t act);
	void action_reset_profiles(uint32_t act);
	void action_save_profile(uint32_t act);
	void action_profile_io(uint32_t act);
	void reload_from_settings();
	void update_input();
	core::ContentArea update_content_area();
	void update_pointer(const core::ContentArea &content);
	/** Combined normal + spotlight scene transform (source space). */
	void scene_view(double *zoom, double *focus_x, double *focus_y) const;
	void update_active_window();
	void update_visual_state(const core::ContentArea &content);
	void render_filter(gs_effect_t *effect);
	void render_texture(gs_texture_t *texture, uint32_t width, uint32_t height,
			    gs_effect_t *effect);
	void sync_parent_cursor(bool hide);
	void restore_parent_cursor();
	void start_zoom_spot(double zx0, double zy0, double zx1, double zy1, bool immediate_out,
			     double hold_override_ms = -1.0);
	void start_locked_highlight_zoom(double dx0, double dy0, double dx1, double dy1);
	void exit_locked_highlight(bool animate);

	/* ---- HotkeyContext (hotkey thread: only queues) ---- */
	void toggle_effects() override { actions_.fetch_or(A_TOGGLE_EFFECTS); }
	void toggle_background() override { actions_.fetch_or(A_TOGGLE_BG); }
	void zoom_in() override { actions_.fetch_or(A_ZOOM_IN); }
	void zoom_out() override { actions_.fetch_or(A_ZOOM_OUT); }
	void reset_zoom() override { actions_.fetch_or(A_RESET_ZOOM); }
	void next_preset() override { actions_.fetch_or(A_NEXT_PRESET); }
	void prev_preset() override { actions_.fetch_or(A_PREV_PRESET); }
	void select_profile(int index) override
	{
		pending_profile_.store(index);
		actions_.fetch_or(A_PROFILE_SET);
	}
	void save_profile() override { actions_.fetch_or(A_SAVE_PROFILE); }
	void toggle_keys() override { actions_.fetch_or(A_TOGGLE_KEYS); }
	void toggle_highlight() override { actions_.fetch_or(A_TOGGLE_HIGHLIGHT); }
	void toggle_spotlight() override { actions_.fetch_or(A_TOGGLE_SPOTLIGHT); }
	void toggle_magnifier() override { actions_.fetch_or(A_TOGGLE_MAGNIFIER); }
	void toggle_cursor() override { actions_.fetch_or(A_TOGGLE_CURSOR); }
	void next_profile() override { actions_.fetch_or(A_PROFILE_NEXT); }
	void prev_profile() override { actions_.fetch_or(A_PROFILE_PREV); }
	void toggle_click_sound() override { actions_.fetch_or(A_TOGGLE_CLICK_SOUND); }
	void reset_profiles() override { actions_.fetch_or(A_RESET_PROFILES); }
	void cancel_highlight() override { actions_.fetch_or(A_CANCEL_HIGHLIGHT); }
	FilterControl *filter_control() override { return this; }

	/* ---- FilterControl (UI thread: only queues / reads atomic) ---- */
	std::string source_name() const override { return source_name_; }
	std::string display_name() const override
	{
		if (source_mode_ || !source)
			return source_name_;
		obs_source_t *parent = obs_filter_get_parent(source);
		const char *parent_name = parent ? obs_source_get_name(parent) : nullptr;
		if (parent_name && *parent_name)
			return std::string(parent_name) + " › " + source_name_;
		return source_name_;
	}
	std::string profile_name(int index) const override
	{
		if (!source)
			return core::profile_label(index);
		obs_data_t *s = obs_source_get_settings(source);
		const std::string name =
			s ? profile_store::display_name(s, index) : core::profile_label(index);
		obs_data_release(s);
		return name;
	}
	void rename_profile(int index, const std::string &name) override
	{
		if (!source || index < 0 || index >= kProfileCount)
			return;
		obs_data_t *s = obs_source_get_settings(source);
		profile_store::store_name(s, index, name);
		/* Persist the slot metadata with the source. */
		obs_source_update(source, s);
		obs_data_release(s);
		refresh_properties();
	}
	void duplicate_profile(int index) override
	{
		pending_duplicate_.store(index);
		actions_.fetch_or(A_DUPLICATE_PROFILE);
	}
	void revert_profile() override
	{
		if (can_revert_profile())
			actions_.fetch_or(A_REVERT_PROFILE);
	}
	bool can_revert_profile() const override
	{
		return profile_baseline_valid_ && profile_edited_.load();
	}
	bool keys_paused() const override { return keys_paused_.load(); }
	void toggle_keys_pause() override { actions_.fetch_or(A_TOGGLE_KEYS); }
	bool keys_enabled() const override { return keys_enabled_cache_.load(); }
	void test_click_sound(bool right) override
	{
		pending_click_test_.store(right ? 1 : 0);
		actions_.fetch_or(A_TEST_CLICK);
	}
	bool effects_enabled() const override { return effects_cache_.load(); }
	void set_effects_enabled(bool on) override
	{
		pending_effects_.store(on ? 1 : 0);
		actions_.fetch_or(A_SET_EFFECTS);
	}
	int preset_count() const override { return presets.count(); }
	std::string preset_name(int index) const override { return presets.name(index); }
	int current_preset() const override { return preset_cache_.load(); }
	void apply_preset(int index) override
	{
		pending_preset_.store(index);
		actions_.fetch_or(A_APPLY_PRESET);
	}
	bool is_user_look(int index) const override { return presets.is_user(index); }
	void save_look(const std::string &name) override
	{
		{
			std::lock_guard<std::mutex> lock(look_mutex_);
			pending_look_name_ = name;
		}
		actions_.fetch_or(A_SAVE_LOOK);
	}
	void delete_look(int index) override
	{
		pending_look_delete_.store(index);
		actions_.fetch_or(A_DELETE_LOOK);
	}
	std::string look_dir() const override { return presets.user_dir(); }
	int input_tier() const override { return input_tier_cache_.load(); }
	bool is_capture_source() const override { return source_mode_; }
	void monitor_origin(double *x, double *y) const override
	{
		*x = monitor_origin_.x();
		*y = monitor_origin_.y();
	}

	/* ---- FilterControl: profiles ---- */
	int profile_count() const override { return kProfileCount; }
	int current_profile() const override { return profile_cache_.load(); }
	bool hotkey_highlight_active() const override { return hotkey_highlight_active_.load(); }
	std::string loaded_sound_path(bool right) const override
	{
		return click_sound.loaded_path(right);
	}
	void export_profiles(const std::string &path,
			     const std::string &scene_profiles_json) override
	{
		{
			std::lock_guard<std::mutex> lock(profile_io_mutex_);
			profile_io_path_ = path;
			profile_io_scenes_ = scene_profiles_json;
		}
		profile_io_result_.store(0);
		actions_.fetch_or(A_EXPORT_PROFILES);
	}
	void import_profiles(const std::string &json_text) override
	{
		{
			std::lock_guard<std::mutex> lock(profile_io_mutex_);
			profile_io_text_ = json_text;
		}
		profile_io_result_.store(0);
		actions_.fetch_or(A_IMPORT_PROFILES);
	}
	int profile_io_result() const override { return profile_io_result_.load(); }
};

} // namespace topscreen::engine_detail
