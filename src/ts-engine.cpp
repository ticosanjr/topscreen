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

using namespace topscreen;
using namespace topscreen::core::keys;

namespace {

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
};

/** Stable fingerprint of the saved config, used to recognize our own pushes. */
std::string config_fingerprint(const core::Config &cfg)
{
	obs_data_t *s = obs_data_create();
	config_store(s, cfg);
	const char *json = obs_data_get_json(s);
	std::string out = json ? json : "";
	obs_data_release(s);
	return out;
}

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

void ts_filter_ctx::push_settings()
{
	obs_data_t *s = obs_source_get_settings(source);
	config_store(s, cfg);
	pushed_fingerprint_ = config_fingerprint(cfg);
	obs_data_set_int(s, kProfileCurrent, current_profile_);

	/* OBS defers the update callback for video sources; the next tick reloads
	 * the values that were just written. This is expected (the reload is
	 * cheap) and keeps the scene-collection state in sync. */
	obs_source_update(source, s);
	obs_data_release(s);
}

void ts_filter_ctx::refresh_properties()
{
	/* OBS keeps the Filters dialog widgets alive; changing settings from the
	 * video thread does not update their current selection. This signal makes
	 * OBS rebuild the view, so cursor/wallpaper/sound dropdowns show the
	 * profile that is actually active. Only call it for bulk changes
	 * (profile/preset/import), never for every slider tick. */
	if (source)
		obs_source_update_properties(source);
}

void ts_filter_ctx::apply_profile(int index)
{
	if (index < 0 || index >= kProfileCount)
		return;
	exit_locked_highlight(false);

	/* Monitor origin is machine-specific, not a look preset: switching
	 * profiles must never move the pointer mapping to another monitor. */
	const bool monitor_manual = cfg.monitor_manual;
	const int monitor_x = cfg.monitor_x;
	const int monitor_y = cfg.monitor_y;

	obs_data_t *s = obs_source_get_settings(source);
	/* Old/partial slots: fields absent from the slot keep that profile's seed
	 * instead of falling back to the global defaults. Missing slots are filled
	 * by ensure_slots() before activation. */
	cfg = profile_store::load_slot(s, index);
	obs_data_release(s);

	cfg.monitor_manual = monitor_manual;
	cfg.monitor_x = monitor_x;
	cfg.monitor_y = monitor_y;

	profile_baseline_ = cfg;
	profile_baseline_valid_ = true;
	profile_edited_.store(false);

	/* Session state: resets the manual zoom and reloads the sound if it changed. */
	zoom.set_manual(1.0);
	zoom.spot_stop();
	hotkey_highlight_active_.store(false);
	/* The active look is now a profile, not the last applied preset. */
	presets.clear_current();
	click_sound.load(cfg.click_sound.file_left.c_str(), cfg.click_sound.file_right.c_str());
}

/**
 * Writes the current config into the slot (without firing update, to avoid
 * feeding the reload -> save -> reload cycle).
 */
void ts_filter_ctx::store_profile_slot(int index)
{
	if (index < 0 || index >= kProfileCount)
		return;

	obs_data_t *s = obs_source_get_settings(source);
	profile_store::store_slot(s, index, cfg);
	obs_data_release(s);
}

void ts_filter_ctx::mark_profile_dirty()
{
	profile_edited_.store(true);
	profile_dirty_ = true;
	profile_dirty_since_ms_ = time_ms;
}

void ts_filter_ctx::flush_profile_if_dirty()
{
	if (!profile_dirty_)
		return;
	store_profile_slot(current_profile_);
	profile_dirty_ = false;
}

void ts_filter_ctx::do_save_profile()
{
	store_profile_slot(current_profile_);
	profile_baseline_ = cfg;
	profile_baseline_valid_ = true;
	profile_edited_.store(false);
	profile_dirty_ = false;
	blog(LOG_INFO, "[TopScreen] profile saved: %s", core::profile_label(current_profile_));
}

void ts_filter_ctx::do_export_profiles(const std::string &path, const std::string &scenes)
{
	/* Make sure the active profile carries the edits that are on screen. */
	store_profile_slot(current_profile_);
	profile_dirty_ = false;

	obs_data_t *s = obs_source_get_settings(source);
	std::string error;
	const bool ok =
		profile_bundle::export_to_file(s, current_profile_, path, scenes, &error);
	obs_data_release(s);

	profile_io_result_.store(ok ? 1 : 2);
	if (ok)
		blog(LOG_INFO, "[TopScreen] profiles exported: %s", path.c_str());
	else
		blog(LOG_WARNING, "[TopScreen] failed to export profiles: %s", error.c_str());
}

void ts_filter_ctx::do_import_profiles(const std::string &json_text)
{
	obs_data_t *s = obs_source_get_settings(source);
	core::ProfileBundleInfo info;
	std::string error;
	/* The scene map is restored by the UI thread before queuing the import;
	 * this thread only touches the filter's own obs_data slots. */
	const bool ok = profile_bundle::import_from_text(json_text, s, &info, nullptr, &error);
	obs_data_release(s);

	if (!ok) {
		profile_io_result_.store(4);
		blog(LOG_WARNING, "[TopScreen] profiles import rejected: %s", error.c_str());
		return;
	}

	/* The imported slots replace everything; a pending debounce write must
	 * not overwrite the freshly imported active profile. */
	profile_dirty_ = false;
	current_profile_ = info.current;
	last_profile = info.current;
	apply_profile(info.current);
	reconfigure_models();
	push_settings();
	refresh_properties();

	profile_io_result_.store(3);
	blog(LOG_INFO, "[TopScreen] profiles imported: %d/%d, current=%s", info.slot_count,
	     kProfileCount, core::profile_label(info.current));
}

/**
 * Resolves which monitor this capture covers, so global cursor coordinates can
 * be turned into content coordinates. Re-resolved when the target size changes
 * and every ~10 s (monitor layouts can change while OBS runs).
 */
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

/**
 * Starts a zoom-spotlight gesture from the model's request. A drawn rectangle
 * is fitted (and centred); a bare hold zooms into the maximum. When the
 * button was already released (immediate_out), the return starts at once.
 */
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

void ts_filter_ctx::request_preset(int index)
{
	if (index < 0)
		return;
	exit_locked_highlight(false);

	/* Flush pending edits first so applying a preset never loses (or bakes in)
	 * the previous slider change. */
	flush_profile_if_dirty();

	std::string err;
	if (!presets.apply(index, &cfg)) {
		blog(LOG_WARNING, "[TopScreen] invalid preset (%d)", index);
		return;
	}

	/* A preset is a temporary look: it is shown immediately but only becomes
	 * part of the active profile when the user saves it (Save profile) or
	 * edits another setting. */
	reconfigure_models();
	click_sound.load(cfg.click_sound.file_left.c_str(), cfg.click_sound.file_right.c_str());
	push_settings();
	refresh_properties();
	blog(LOG_INFO, "[TopScreen] preset applied: %s (temporary until saved)", presets.name(index).c_str());
}

void ts_filter_ctx::reload_from_settings()
{
	const int old_highlight_mode = cfg.highlight.mode;
	obs_data_t *s = obs_source_get_settings(source);
	config_load(s, &cfg);
	const long long pc = obs_data_get_int(s, kProfileCurrent);
	obs_data_release(s);

	/* A reload that matches the values we just pushed is our own update; it
	 * must not be mistaken for a user edit. */
	const bool self_update =
		!pushed_fingerprint_.empty() && config_fingerprint(cfg) == pushed_fingerprint_;
	pushed_fingerprint_.clear();

	/* A setting edit can change/disable the active drag mode. A pinned
	 * highlight must not survive that; ordinary edits (e.g. click sound) do. */
	if (highlight.locked() &&
	    (cfg.highlight.mode != old_highlight_mode || !cfg.highlight.enabled ||
	     !cfg.effects_enabled))
		exit_locked_highlight(false);

	const int want = std::max(0, std::min(kProfileCount - 1, static_cast<int>(pc)));

	if (static_cast<int>(pc) != last_profile) {
		/*
		 * Profile switched (Properties list): applies the ENTIRE slot and
		 * syncs the main keys. Flush a pending auto-save to the OLD profile
		 * first, so a slider edit immediately followed by a profile change
		 * is not lost. Without the push, the next reload (any other
		 * property) would reload the old keys and undo the profile.
		 */
		flush_profile_if_dirty();
		last_profile = static_cast<int>(pc);
		current_profile_ = want;
		apply_profile(current_profile_);
		reconfigure_models();
		push_settings();
		refresh_properties();
		blog(LOG_INFO, "[TopScreen] profile activated: %s", core::profile_label(current_profile_));
		return;
	}

	/*
	 * Normal edit: the active profile follows what is on screen (auto-save),
	 * so the user's changes are not lost when switching profiles. The write
	 * is debounced: slider drags update cfg every tick, and storing the
	 * whole slot on each one is unnecessary I/O.
	 */
	current_profile_ = want;
	reconfigure_models();
	/* Reloads the click sounds when the selection changed (slots keep the
	 * unchanged side, so this is cheap). */
	click_sound.load(cfg.click_sound.file_left.c_str(), cfg.click_sound.file_right.c_str());
	if (!self_update)
		mark_profile_dirty();
}

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

/* ------------------------------------------------------------------ */

void *create_engine_impl(obs_data_t *settings, obs_source_t *source, bool source_mode)
{
	auto *ctx = new ts_filter_ctx();
	ctx->source = source;
	ctx->source_mode_ = source_mode;
	const char *source_name = obs_source_get_name(source);
	ctx->source_name_ = source_name ? source_name : "";

	config_load(settings, &ctx->cfg);

	const long long pc = obs_data_get_int(settings, kProfileCurrent);
	ctx->current_profile_ = std::max(0, std::min(kProfileCount - 1, static_cast<int>(pc)));
	ctx->last_profile = ctx->current_profile_;
	ctx->profile_cache_.store(ctx->current_profile_);

	/* Guarantees the 10 profile slots. A source from an older build is
	 * reseeded once (the user asked for the new A..J defaults); after the
	 * version is current, later edits are preserved. */
	bool settings_changed = false;
	{
		obs_data_t *s = obs_source_get_settings(source);
		settings_changed = profile_store::ensure_slots(s, kProfileSeedRevision);
		if (settings_changed)
			obs_source_update(source, s);
		obs_data_release(s);
	}

	/*
	 * If the user already chose a profile (user value in profile_current,
	 * e.g.: saved collection), the source state IS the profile. Without this, reopening OBS
	 * left "Profile B" selected with the default look.
	 */
	const bool picked = obs_data_has_user_value(settings, kProfileCurrent);
	if (picked || settings_changed) {
		ctx->apply_profile(ctx->current_profile_);
		ctx->push_settings();
	}

	ctx->reconfigure_models();

	ctx->presets.load();
	ctx->click_sound.load(ctx->cfg.click_sound.file_left.c_str(),
				 ctx->cfg.click_sound.file_right.c_str());

	ctx->loaded = ctx->compositor.load();
	if (!ctx->loaded)
		blog(LOG_WARNING, "[TopScreen] failed to load the effect (passthrough)");

	ctx->cursor.reset(0.5, 0.5, 0.0);
	ctx->zoom.reset(0.0);

	ctx->effects_cache_.store(ctx->cfg.effects_enabled);

	ctx->hotkeys.register_all(ctx);
	ctx->input.start(source);

	registry_add(ctx);
	/* A filter created while OBS is already on a mapped scene must start with
	 * that profile; waiting for the next SCENE_CHANGED would miss the scene
	 * that is active at startup. Queued and applied on the next tick. */
	Hotkeys::apply_scene_profile_to_current();

	blog(LOG_INFO, "[TopScreen] %s created (presets=%d)",
	     source_mode ? "capture engine" : "filter", ctx->presets.count());

	return ctx;
}

void destroy_engine_impl(void *data)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);

	ctx->flush_profile_if_dirty();
	ctx->restore_parent_cursor();
	registry_remove(ctx);
	ctx->input.stop();
	ctx->hotkeys.unregister_all(ctx);
	ctx->click_sound.release();
	ctx->compositor.release();

	delete ctx;
}

void update_engine_impl(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	if (!ctx)
		return;
	/* Does not touch cfg here (UI thread); the tick reloads. */
	ctx->pending_reload_.store(true);
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

void tick_engine_impl(void *data, float seconds)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	ctx->time_ms += static_cast<double>(seconds) * 1000.0;

	if (ctx->pending_reload_.exchange(false))
		ctx->reload_from_settings();

	ctx->process_actions();

	/* Debounced profile auto-save (slider drags update cfg every tick). */
	if (ctx->profile_dirty_ &&
	    ctx->time_ms - ctx->profile_dirty_since_ms_ >= ts_filter_ctx::kProfileSaveDebounceMs)
		ctx->flush_profile_if_dirty();

	ctx->monitor_origin_.tick(ctx->source, ctx->cfg, ctx->target_w, ctx->target_h);
	ctx->update_input();
	const core::ContentArea content = ctx->update_content_area();
	ctx->update_pointer(content);
	ctx->update_visual_state(content);
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

} // namespace

namespace topscreen {

bool profiles_export_from_properties(void *filter)
{
	auto *ctx = static_cast<ts_filter_ctx *>(filter);
	if (!ctx || !ctx->source)
		return false;

	obs_data_t *s = obs_source_get_settings(ctx->source);
	std::string path = s ? obs_data_get_string(s, kProfileExportPath) : "";
	obs_data_release(s);

	if (path.empty()) {
		blog(LOG_WARNING, "[TopScreen] profiles export: choose a file first");
		return false;
	}
	ctx->export_profiles(path, scene_profiles::export_json());
	return true;
}

bool profiles_reset_from_properties(void *filter)
{
	auto *ctx = static_cast<ts_filter_ctx *>(filter);
	if (!ctx || !ctx->source)
		return false;
	ctx->reset_profiles();
	return true;
}

bool profiles_rename_from_properties(void *filter)
{
	auto *ctx = static_cast<ts_filter_ctx *>(filter);
	if (!ctx || !ctx->source)
		return false;
	obs_data_t *s = obs_source_get_settings(ctx->source);
	std::string name = s ? obs_data_get_string(s, kProfileNameInput) : "";
	const int current = s ? static_cast<int>(obs_data_get_int(s, kProfileCurrent))
			      : ctx->current_profile();
	obs_data_release(s);
	ctx->rename_profile(current, name);
	return true;
}

bool profiles_duplicate_from_properties(void *filter)
{
	auto *ctx = static_cast<ts_filter_ctx *>(filter);
	if (!ctx || !ctx->source)
		return false;
	obs_data_t *s = obs_source_get_settings(ctx->source);
	const int target = s ? static_cast<int>(obs_data_get_int(s, kProfileDuplicateTarget)) : -1;
	obs_data_release(s);
	ctx->duplicate_profile(target);
	return true;
}

bool profiles_revert_from_properties(void *filter)
{
	auto *ctx = static_cast<ts_filter_ctx *>(filter);
	if (!ctx)
		return false;
	ctx->revert_profile();
	return true;
}

bool profiles_overwrite_confirmed(void *filter)
{
	auto *ctx = static_cast<ts_filter_ctx *>(filter);
	if (!ctx || !ctx->source)
		return false;
	obs_data_t *s = obs_source_get_settings(ctx->source);
	const bool confirmed = s && obs_data_get_bool(s, kProfileConfirmOverwrite);
	obs_data_release(s);
	return confirmed;
}

bool profiles_import_from_properties(void *filter)
{
	auto *ctx = static_cast<ts_filter_ctx *>(filter);
	if (!ctx || !ctx->source)
		return false;

	obs_data_t *s = obs_source_get_settings(ctx->source);
	std::string path = s ? obs_data_get_string(s, kProfileImportPath) : "";
	obs_data_release(s);

	if (path.empty()) {
		blog(LOG_WARNING, "[TopScreen] profiles import: choose a file first");
		return false;
	}

	std::string text;
	if (!core::read_file_text(path.c_str(), &text, core::kMaxProfileBundleBytes)) {
		blog(LOG_WARNING, "[TopScreen] profiles import: could not read %s", path.c_str());
		return false;
	}

	core::ProfileBundleInfo info;
	std::string error;
	if (!core::profile_bundle_validate(text, &info, &error)) {
		blog(LOG_WARNING, "[TopScreen] profiles import rejected: %s", error.c_str());
		return false;
	}

	/* The scene map is config/UI state, so restore it on this thread instead of
	 * touching the OBS profile config from the video thread. */
	if (!scene_profiles::import_from_bundle(text))
		blog(LOG_WARNING, "[TopScreen] profiles import: invalid scene profile map");
	ctx->import_profiles(text);
	return true;
}

} // namespace topscreen

namespace topscreen {

void *ts_engine_create(obs_data_t *settings, obs_source_t *source, bool source_mode)
{
	return create_engine_impl(settings, source, source_mode);
}

void ts_engine_destroy(void *engine)
{
	destroy_engine_impl(engine);
}

void ts_engine_update(void *engine, obs_data_t *settings)
{
	update_engine_impl(engine, settings);
}

void ts_engine_defaults(obs_data_t *settings)
{
	config_defaults(settings);
}

void ts_engine_set_target(void *engine, uint32_t width, uint32_t height)
{
	auto *ctx = static_cast<ts_filter_ctx *>(engine);
	if (!ctx)
		return;
	ctx->target_w = width;
	ctx->target_h = height;
}

void ts_engine_tick(void *engine, float seconds)
{
	tick_engine_impl(engine, seconds);
}

void ts_engine_render_filter(void *engine, gs_effect_t *effect)
{
	auto *ctx = static_cast<ts_filter_ctx *>(engine);
	if (ctx)
		ctx->render_filter(effect);
}

void ts_engine_render_texture(void *engine, gs_texture_t *texture, uint32_t width,
			      uint32_t height, gs_effect_t *effect)
{
	auto *ctx = static_cast<ts_filter_ctx *>(engine);
	if (ctx)
		ctx->render_texture(texture, width, height, effect);
}

obs_properties_t *ts_engine_properties(void *engine, obs_source_t *source,
				       void (*prefix)(obs_properties_t *, void *),
				       void *prefix_data)
{
	auto *ctx = static_cast<ts_filter_ctx *>(engine);
	if (!ctx)
		return obs_properties_create();
	obs_source_t *host = source ? source : ctx->source;
	obs_data_t *settings = host ? obs_source_get_settings(host) : nullptr;
	obs_properties_t *props =
		properties_create(engine, settings, !ctx->source_mode_, prefix, prefix_data);
	obs_data_release(settings);
	return props;
}

} // namespace topscreen
