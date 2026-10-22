#include "ts-filter.hpp"

#include "audio/ts-click-sound.hpp"
#include "core/coord.hpp"
#include "core/file.hpp"
#include "core/keys.hpp"
#include "core/math.hpp"
#include "core/geometry.hpp"
#include "core/profiles.hpp"
#include "core/profiles_io.hpp"
#include "core/spotlight.hpp"
#include "hotkeys/ts-hotkeys.hpp"
#include "input/ts-input.hpp"
#include "input/ts-monitor.hpp"
#include "render/ts-compositor.hpp"
#include "render/ts-frame.hpp"
#include "ts-presets.hpp"
#include "ts-profiles-ui.hpp"
#include "ts-properties.hpp"
#include "ts-registry.hpp"
#include "ts-settings.hpp"
#include "ts-version.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
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
};

/* Native cursor ownership is shared by every TopScreen filter attached to the
 * same capture. Each filter contributes one "hide" want while effects and its
 * own cursor are enabled; the capture is restored only when the last want goes
 * away. This avoids one filter restoring the cursor while another still draws
 * its own. */
struct CursorEntry {
	int users = 0;
	int hidden_wants = 0;
	bool has[2] = {false, false};
	bool was[2] = {false, false};
	bool logged_missing = false;
};

std::mutex cursor_mutex;
std::map<obs_source_t *, CursorEntry> cursor_entries;

const char *const kCursorKeys[] = {"capture_cursor", "show_cursor"};

void capture_cursor_state(obs_source_t *parent, CursorEntry *entry)
{
	obs_properties_t *props = obs_source_properties(parent);
	if (!props)
		return;
	for (int i = 0; i < 2; ++i)
		entry->has[i] = obs_properties_get(props, kCursorKeys[i]) != nullptr;
	obs_properties_destroy(props);

	obs_data_t *s = obs_source_get_settings(parent);
	if (s) {
		for (int i = 0; i < 2; ++i) {
			if (entry->has[i])
				entry->was[i] = obs_data_get_bool(s, kCursorKeys[i]);
		}
		obs_data_release(s);
	}

	if (!entry->has[0] && !entry->has[1] && !entry->logged_missing) {
		entry->logged_missing = true;
		blog(LOG_INFO, "[TopScreen] parent source does not expose a native cursor option (%s)",
		     obs_source_get_name(parent) ? obs_source_get_name(parent) : "?");
	}
}

void apply_cursor_state(obs_source_t *parent, CursorEntry *entry)
{
	obs_data_t *s = obs_source_get_settings(parent);
	if (!s)
		return;

	const bool hide = entry->hidden_wants > 0;
	bool changed = false;
	for (int i = 0; i < 2; ++i) {
		if (!entry->has[i])
			continue;
		const bool want = hide ? false : entry->was[i];
		const bool current = obs_data_get_bool(s, kCursorKeys[i]);
		if (current == want)
			continue;
		obs_data_set_bool(s, kCursorKeys[i], want);
		changed = true;
		blog(LOG_INFO, "[TopScreen] capture native cursor %s (%s, was %s)",
		     want ? "restored" : "disabled", kCursorKeys[i], current ? "on" : "off");
	}
	if (changed)
		obs_source_update(parent, s);
	obs_data_release(s);
}

void acquire_parent_cursor(obs_source_t *parent)
{
	std::lock_guard<std::mutex> lock(cursor_mutex);
	CursorEntry &entry = cursor_entries[parent];
	if (entry.users++ == 0)
		capture_cursor_state(parent, &entry);
}

void set_parent_cursor_wanted(obs_source_t *parent, bool hide)
{
	std::lock_guard<std::mutex> lock(cursor_mutex);
	auto it = cursor_entries.find(parent);
	if (it == cursor_entries.end())
		return;
	CursorEntry &entry = it->second;
	if (hide)
		++entry.hidden_wants;
	else if (entry.hidden_wants > 0)
		--entry.hidden_wants;
	apply_cursor_state(parent, &entry);
}

void release_parent_cursor(obs_source_t *parent, bool was_hiding)
{
	std::lock_guard<std::mutex> lock(cursor_mutex);
	auto it = cursor_entries.find(parent);
	if (it == cursor_entries.end())
		return;
	CursorEntry &entry = it->second;
	if (was_hiding && entry.hidden_wants > 0)
		--entry.hidden_wants;
	if (entry.users > 0)
		--entry.users;
	if (entry.users == 0) {
		entry.hidden_wants = 0;
		apply_cursor_state(parent, &entry);
		cursor_entries.erase(it);
	}
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
	FrameState frame;

	uint32_t target_w = 0;
	uint32_t target_h = 0;
	double time_ms = 0.0;
	bool loaded = false;
	obs_source_t *cursor_parent_ = nullptr; // parent that owns the cursor lease
	bool cursor_acquired_ = false;
	bool cursor_hiding_ = false;
	bool prev_left = false;
	std::atomic<bool> hotkey_highlight_active_{false};

	int current_profile_ = 0;
	int last_profile = -1;
	/* Auto-save debounce: a slider drag must not write the profile slot on
	 * every tick. Flushed on profile switch/save/export/destroy. */
	bool profile_dirty_ = false;
	double profile_dirty_since_ms_ = 0.0;
	static constexpr double kProfileSaveDebounceMs = 400.0;

	/* Origin of the captured monitor in GLOBAL desktop coordinates: the input
	 * reports global positions, so a monitor below/left of the primary one
	 * needs its offset subtracted before normalizing. */
	std::atomic<double> origin_x_{0.0};
	std::atomic<double> origin_y_{0.0};
	std::atomic<bool> origin_known_{false};
	uint32_t origin_target_w_ = 0;
	uint32_t origin_target_h_ = 0;
	int origin_age_ticks_ = 0;

	std::atomic<uint32_t> actions_{0};
	std::atomic<int> pending_effects_{-1};
	std::atomic<int> pending_preset_{-1};
	std::atomic<int> pending_profile_{-1};
	std::atomic<bool> pending_reload_{false};
	std::atomic<bool> effects_cache_{true};
	std::atomic<int> preset_cache_{-1};
	std::atomic<int> profile_cache_{0};
	std::atomic<int> input_tier_cache_{0};

	/* Profiles backup requests (UI thread -> video thread). */
	std::mutex profile_io_mutex_;
	std::string profile_io_path_;
	std::string profile_io_text_;
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
	void do_export_profiles(const std::string &path);
	void do_import_profiles(const std::string &json_text);
	void process_actions();
	void reload_from_settings();
	void refresh_monitor_origin(bool force);
	void sync_parent_cursor(bool hide);
	void restore_parent_cursor();
	void start_zoom_spot(double zx0, double zy0, double zx1, double zy1, bool immediate_out,
			     double hold_override_ms = -1.0);

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
	void toggle_highlight() override { actions_.fetch_or(A_TOGGLE_HIGHLIGHT); }
	void toggle_spotlight() override { actions_.fetch_or(A_TOGGLE_SPOTLIGHT); }
	FilterControl *filter_control() override { return this; }

	/* ---- FilterControl (UI thread: only queues / reads atomic) ---- */
	std::string source_name() const override { return source_name_; }
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
	int input_tier() const override { return input_tier_cache_.load(); }
	void monitor_origin(double *x, double *y) const override
	{
		*x = origin_known_.load() ? origin_x_.load() : 0.0;
		*y = origin_known_.load() ? origin_y_.load() : 0.0;
	}

	/* ---- FilterControl: profiles ---- */
	int profile_count() const override { return kProfileCount; }
	int current_profile() const override { return profile_cache_.load(); }
	bool hotkey_highlight_active() const override { return hotkey_highlight_active_.load(); }
	std::string loaded_sound_path(bool right) const override
	{
		return click_sound.loaded_path(right);
	}
	void export_profiles(const std::string &path) override
	{
		{
			std::lock_guard<std::mutex> lock(profile_io_mutex_);
			profile_io_path_ = path;
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

	obs_data_t *s = obs_source_get_settings(source);
	obs_data_t *p = obs_data_get_obj(s, kProfileSlots[index]);
	if (p) {
		/* Old/partial slots: fields absent from the slot keep that profile's
		 * seed instead of falling back to the global defaults. */
		const core::Config seed = core::make_profile_seed(index);
		config_load(p, &cfg, &seed);
		obs_data_release(p);
	} else {
		/* Empty slot (legacy source): capture the current state instead of doing
		 * nothing, otherwise switching profiles would look like "not working". */
		p = obs_data_create();
		config_store(p, cfg);
		obs_data_set_obj(s, kProfileSlots[index], p);
		obs_data_release(p);
	}
	obs_data_release(s);

	/* Session state: resets the manual zoom and reloads the sound if it changed. */
	zoom.set_manual(1.0);
	zoom.spot_stop();
	hotkey_highlight_active_.store(false);
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
	obs_data_t *p = obs_data_get_obj(s, kProfileSlots[index]);
	if (!p) {
		p = obs_data_create();
		obs_data_set_obj(s, kProfileSlots[index], p);
	}
	config_store(p, cfg);
	obs_data_release(p);
	obs_data_release(s);
}

void ts_filter_ctx::mark_profile_dirty()
{
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
	profile_dirty_ = false;
	blog(LOG_INFO, "[TopScreen] profile saved: %s", core::profile_label(current_profile_));
}

void ts_filter_ctx::do_export_profiles(const std::string &path)
{
	if (path.empty()) {
		profile_io_result_.store(2);
		blog(LOG_WARNING, "[TopScreen] profiles export: empty path");
		return;
	}

	/* Make sure the active profile carries the edits that are on screen. */
	store_profile_slot(current_profile_);
	profile_dirty_ = false;

	obs_data_t *s = obs_source_get_settings(source);
	obs_data_t *bundle = obs_data_create();
	obs_data_set_int(bundle, "topscreen_profiles", 1);
	obs_data_set_string(bundle, "plugin", "TopScreen");
	obs_data_set_string(bundle, "version", kVersion);
	obs_data_set_int(bundle, "current", current_profile_);
	for (int i = 0; i < kProfileCount; ++i) {
		obs_data_t *slot = obs_data_get_obj(s, kProfileSlots[i]);
		/* Normalize every slot: older sources can miss fields added in a
		 * newer version, and the exported bundle must always be complete. */
		const core::Config seed = core::make_profile_seed(i);
		core::Config normalized = seed;
		if (slot) {
			config_load(slot, &normalized, &seed);
			obs_data_release(slot);
		}
		obs_data_t *out_slot = obs_data_create();
		config_store(out_slot, normalized);
		obs_data_set_obj(bundle, kProfileSlots[i], out_slot);
		obs_data_release(out_slot);
	}
	obs_data_release(s);

	const bool ok = obs_data_save_json_pretty_safe(bundle, path.c_str(), "tmp", "bak");
	obs_data_release(bundle);

	profile_io_result_.store(ok ? 1 : 2);
	if (ok)
		blog(LOG_INFO, "[TopScreen] profiles exported: %s", path.c_str());
	else
		blog(LOG_WARNING, "[TopScreen] failed to export profiles: %s", path.c_str());
}

void ts_filter_ctx::do_import_profiles(const std::string &json_text)
{
	core::ProfileBundleInfo info;
	std::string error;
	if (!core::profile_bundle_validate(json_text, &info, &error)) {
		profile_io_result_.store(4);
		blog(LOG_WARNING, "[TopScreen] profiles import rejected: %s", error.c_str());
		return;
	}

	obs_data_t *bundle = obs_data_create_from_json(json_text.c_str());
	if (!bundle) {
		profile_io_result_.store(4);
		blog(LOG_WARNING, "[TopScreen] profiles import: could not parse the JSON");
		return;
	}

	obs_data_t *s = obs_source_get_settings(source);
	for (int i = 0; i < kProfileCount; ++i) {
		obs_data_t *slot = obs_data_get_obj(bundle, kProfileSlots[i]);
		/* Start from the slot's seed and overlay the fields the imported file
		 * actually has. Missing slots keep the seed; missing fields keep the
		 * seed's value instead of becoming 0/false. This keeps old/new bundles
		 * valid across added or removed settings. */
		const core::Config seed = core::make_profile_seed(i);
		core::Config normalized = seed;
		if (slot) {
			config_load(slot, &normalized, &seed);
			obs_data_release(slot);
		}
		obs_data_t *out_slot = obs_data_create();
		config_store(out_slot, normalized);
		obs_data_set_obj(s, kProfileSlots[i], out_slot);
		obs_data_release(out_slot);
	}
	obs_data_set_int(s, kProfileCurrent, info.current);
	obs_data_release(s);
	obs_data_release(bundle);

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
void ts_filter_ctx::refresh_monitor_origin(bool force)
{
	if (target_w == 0 || target_h == 0)
		return;

	/* Manual wins: it is the escape hatch when the platform detection cannot
	 * decide (identical monitors stacked below/above each other). */
	if (cfg.monitor_manual) {
		const double nx = static_cast<double>(cfg.monitor_x);
		const double ny = static_cast<double>(cfg.monitor_y);
		if (force || !origin_known_.load() || nx != origin_x_.load() ||
		    ny != origin_y_.load()) {
			origin_x_.store(nx);
			origin_y_.store(ny);
			origin_known_.store(true);
			blog(LOG_INFO, "[TopScreen] monitor origin: (%.0f,%.0f) [manual]", nx, ny);
		}
		origin_age_ticks_ = 0;
		return;
	}

	const bool size_changed = target_w != origin_target_w_ || target_h != origin_target_h_;
	if (!force && !size_changed && origin_age_ticks_ < 600)
		return;

	origin_age_ticks_ = 0;
	origin_target_w_ = target_w;
	origin_target_h_ = target_h;

	/* The capture settings expose the monitor index on Windows (same order as
	 * EnumDisplayMonitors). X11's "screen" is an X screen, not a monitor, so
	 * the hint is not trusted there. */
	int hint = -1;
	bool trust_hint = false;
	std::string device;
	if (obs_source_t *parent = obs_filter_get_parent(source)) {
		obs_data_t *s = obs_source_get_settings(parent);
		if (s) {
			/* Newer OBS identifies the monitor by device name
			 * (alt_id/id, e.g. "\\.\DISPLAY2"); older builds use an
			 * integer index. Both are optional. */
			for (const char *key : {"alt_id", "device_id", "id"}) {
				const char *v = obs_data_get_string(s, key);
				if (v && *v && std::strncmp(v, "\\\\", 2) == 0) {
					device = v;
					break;
				}
			}
			if (obs_data_has_user_value(s, "monitor") ||
			    obs_data_has_default_value(s, "monitor")) {
				hint = static_cast<int>(obs_data_get_int(s, "monitor"));
				trust_hint = true;
			}
			obs_data_release(s);
		}
	}

	double ox = 0.0;
	double oy = 0.0;
	const bool known = ts_resolve_monitor_origin(device.c_str(), hint, trust_hint, target_w,
						     target_h, &ox, &oy);
	origin_x_.store(known ? ox : 0.0);
	origin_y_.store(known ? oy : 0.0);
	origin_known_.store(known);
}

void ts_filter_ctx::sync_parent_cursor(bool hide)
{
	obs_source_t *parent = obs_filter_get_parent(source);

	if (parent != cursor_parent_) {
		if (cursor_parent_ && cursor_acquired_)
			release_parent_cursor(cursor_parent_, cursor_hiding_);
		cursor_parent_ = parent;
		cursor_acquired_ = false;
		cursor_hiding_ = false;
		if (parent) {
			acquire_parent_cursor(parent);
			cursor_acquired_ = true;
		}
	}

	if (!cursor_acquired_ || hide == cursor_hiding_)
		return;
	set_parent_cursor_wanted(cursor_parent_, hide);
	cursor_hiding_ = hide;
}

void ts_filter_ctx::restore_parent_cursor()
{
	obs_source_t *parent = obs_filter_get_parent(source);
	if (parent && parent == cursor_parent_ && cursor_acquired_)
		release_parent_cursor(cursor_parent_, cursor_hiding_);
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
	const int dir = cfg.highlight.zoom_dir;
	const core::SpotAim aim =
		core::spot_aim_for_hold(has_rect, zx0, zy0, zx1, zy1, (zx0 + zx1) * 0.5,
					(zy0 + zy1) * 0.5, base, cfg.highlight.zoom_max,
					cfg.highlight.zoom_max, dir == 1 || dir == 3);
	if (!aim.valid)
		return;

	/* Punch modes skip the hold: snap in now, glide straight back out. */
	const bool punch = dir >= 2;
	const double hold_ms = hold_override_ms >= 0.0 ? hold_override_ms
						       : cfg.highlight.zoom_hold_ms;
	zoom.spot_start_hold(aim.focus_x, aim.focus_y, aim.zoom, cfg.highlight.zoom_in_ms,
			     (punch || immediate_out) ? 0.0 : hold_ms,
			     cfg.highlight.zoom_out_ms, time_ms);
	blog(LOG_INFO, "[TopScreen] zoom spotlight: dir=%d %.2fx%s", dir, aim.zoom,
	     immediate_out ? " (quick)" : "");
	if (immediate_out)
		zoom.spot_release(time_ms);
}

void ts_filter_ctx::reconfigure_models()
{
	cursor.configure(cfg.cursor);
	zoom.configure(cfg.zoom);
	clicks.set_left(cfg.click_left);
	clicks.set_right(cfg.click_right);
	keys.configure(cfg.keys);
	highlight.configure(cfg.highlight);
	spotlight.configure(cfg.spotlight);
	spotlight.set_active(cfg.spotlight.enabled, time_ms);
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

	std::string err;
	if (!presets.apply(index, &cfg)) {
		blog(LOG_WARNING, "[TopScreen] invalid preset (%d)", index);
		return;
	}

	reconfigure_models();
	click_sound.load(cfg.click_sound.file_left.c_str(), cfg.click_sound.file_right.c_str());
	push_settings();
	refresh_properties();
	mark_profile_dirty();
	blog(LOG_INFO, "[TopScreen] preset applied: %s", presets.name(index).c_str());
}

void ts_filter_ctx::reload_from_settings()
{
	obs_data_t *s = obs_source_get_settings(source);
	config_load(s, &cfg);
	const long long pc = obs_data_get_int(s, kProfileCurrent);
	obs_data_release(s);

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
	mark_profile_dirty();
}

void ts_filter_ctx::process_actions()
{
	const uint32_t act = actions_.exchange(0);
	if (act == 0)
		return;

	if (act & A_TOGGLE_EFFECTS) {
		cfg.effects_enabled = !cfg.effects_enabled;
		push_settings();
		mark_profile_dirty();
	}
	if (act & A_SET_EFFECTS) {
		const int v = pending_effects_.exchange(-1);
		if (v >= 0) {
			cfg.effects_enabled = (v != 0);
			push_settings();
			mark_profile_dirty();
		}
	}
	if (act & A_TOGGLE_BG) {
		cfg.bg.enabled = !cfg.bg.enabled;
		push_settings();
		mark_profile_dirty();
	}
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
	if (act & A_PROFILE_SET) {
		const int i = pending_profile_.exchange(-1);
		if (i >= 0 && i < kProfileCount) {
			/* Save pending edits to the profile we are leaving. */
			flush_profile_if_dirty();
			current_profile_ = i;
			last_profile = i;
			apply_profile(i);
			reconfigure_models();
			push_settings();
			refresh_properties();
			blog(LOG_INFO, "[TopScreen] profile activated: %s", core::profile_label(i));
		}
	}
	if (act & A_TOGGLE_HIGHLIGHT) {
		if (!cfg.effects_enabled || !cfg.highlight.hotkey_enabled) {
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
			/* First press: zoom into the current cursor position. */
			const double cx = frame.cursor.x;
			const double cy = frame.cursor.y;
			start_zoom_spot(cx, cy, cx, cy, false, 0.0);
			hotkey_highlight_active_.store(zoom.spot_active());
			blog(LOG_INFO, "[TopScreen] hotkey highlight: %s",
			     hotkey_highlight_active_.load() ? "active" : "no-op");
		}
	}
	if (act & A_TOGGLE_SPOTLIGHT) {
		cfg.spotlight.enabled = !cfg.spotlight.enabled;
		spotlight.set_active(cfg.spotlight.enabled, time_ms);
		push_settings();
		mark_profile_dirty();
		blog(LOG_INFO, "[TopScreen] spotlight: %s", cfg.spotlight.enabled ? "on" : "off");
	}
	if (act & A_SAVE_PROFILE)
		do_save_profile();
	if (act & (A_EXPORT_PROFILES | A_IMPORT_PROFILES)) {
		std::string path;
		std::string text;
		{
			std::lock_guard<std::mutex> lock(profile_io_mutex_);
			path.swap(profile_io_path_);
			text.swap(profile_io_text_);
		}
		if (act & A_EXPORT_PROFILES)
			do_export_profiles(path);
		if (act & A_IMPORT_PROFILES)
			do_import_profiles(text);
	}
}

/* ------------------------------------------------------------------ */

const char *ts_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FilterName");
}

void *ts_filter_create(obs_data_t *settings, obs_source_t *source)
{
	auto *ctx = new ts_filter_ctx();
	ctx->source = source;
	const char *source_name = obs_source_get_name(source);
	ctx->source_name_ = source_name ? source_name : "";

	config_load(settings, &ctx->cfg);

	const long long pc = obs_data_get_int(settings, kProfileCurrent);
	ctx->current_profile_ = std::max(0, std::min(kProfileCount - 1, static_cast<int>(pc)));
	ctx->last_profile = ctx->current_profile_;
	ctx->profile_cache_.store(ctx->current_profile_);

	/* Guarantees the 10 profile slots (seeds visible variations into the empty ones). */
	{
		obs_data_t *s = obs_source_get_settings(source);
		for (int i = 0; i < kProfileCount; ++i) {
			if (obs_data_get_obj(s, kProfileSlots[i]))
				continue;
			obs_data_t *p = obs_data_create();
			const core::Config seed = core::make_profile_seed(i);
			config_store(p, seed);
			obs_data_set_obj(s, kProfileSlots[i], p);
			obs_data_release(p);
		}
		obs_source_update(source, s);
		obs_data_release(s);
	}

	/*
	 * If the user already chose a profile (user value in profile_current,
	 * e.g.: saved collection), the source state IS the profile. Without this, reopening OBS
	 * left "Profile B" selected with the default look.
	 */
	const bool picked = obs_data_has_user_value(settings, kProfileCurrent);
	if (picked) {
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

	blog(LOG_INFO, "[TopScreen] filter created (presets=%d)", ctx->presets.count());

	return ctx;
}

void ts_filter_destroy(void *data)
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

void ts_filter_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	if (!ctx)
		return;
	/* Does not touch cfg here (UI thread); the tick reloads. */
	ctx->pending_reload_.store(true);
}

void ts_filter_tick(void *data, float seconds)
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

	if (ctx->target_w > 0 && ctx->target_h > 0) {
		++ctx->origin_age_ticks_;
		ctx->refresh_monitor_origin(false);
	}

	ctx->input.poll();

	/* Keys feed the overlay even without a cursor available. */
	for (int i = 0; i < ctx->input.state().key_count; ++i)
		ctx->keys.press(ctx->input.state().key_labels[i], ctx->input.state().key_modifier[i],
				ctx->time_ms);

	if (ctx->input.available() && ctx->target_w > 0 && ctx->target_h > 0) {
		const InputState &st = ctx->input.state();
		const core::Monitor mon{ctx->origin_known_.load() ? ctx->origin_x_.load() : 0.0,
					ctx->origin_known_.load() ? ctx->origin_y_.load() : 0.0,
					static_cast<double>(ctx->target_w),
					static_cast<double>(ctx->target_h)};
		const core::Vec2f c = core::screen_to_content(st.x, st.y, mon);

		const double cx = core::clamp01(c.x);
		const double cy = core::clamp01(c.y);

		ctx->cursor.set_target(cx, cy, ctx->time_ms);
		ctx->zoom.set_cursor(cx, cy, ctx->time_ms);

		/* Drag highlighter (left button). A drag owns the zoom spot now, so a
		 * hotkey toggle must not leave a stale "active" state behind. */
		if (st.left_pressed) {
			if (ctx->hotkey_highlight_active_.exchange(false))
				ctx->zoom.spot_release(ctx->time_ms);
			ctx->highlight.begin(cx, cy, ctx->time_ms);
		} else if (st.left)
			ctx->highlight.move(cx, cy, ctx->time_ms);
		if (!st.left && ctx->prev_left) {
			ctx->highlight.end(ctx->time_ms);

			/* Released before the trigger (or QuickZoom-style tap-drag):
			 * if the model still holds a request, run the in/out cycle now. */
			double zx0 = 0.0, zy0 = 0.0, zx1 = 0.0, zy1 = 0.0;
			if (ctx->highlight.take_zoom_request(&zx0, &zy0, &zx1, &zy1)) {
				ctx->hotkey_highlight_active_.store(false);
				ctx->start_zoom_spot(zx0, zy0, zx1, zy1, true);
			}
		} else if (st.left) {
			/* While held: fire the zoom as soon as the model asks for it. */
			double zx0 = 0.0, zy0 = 0.0, zx1 = 0.0, zy1 = 0.0;
			if (ctx->highlight.take_zoom_request(&zx0, &zy0, &zx1, &zy1)) {
				ctx->hotkey_highlight_active_.store(false);
				ctx->start_zoom_spot(zx0, zy0, zx1, zy1, false);
			}
		}

		/* Let go: glide back with the configured out animation. */
		if (!st.left && ctx->prev_left) {
			ctx->hotkey_highlight_active_.store(false);
			ctx->zoom.spot_release(ctx->time_ms);
		}

		ctx->prev_left = st.left;

		if (st.left_pressed) {
			ctx->clicks.spawn(cx, cy, ctx->time_ms, false);
			if (ctx->cfg.click_sound.enabled && ctx->cfg.click_sound.left)
				ctx->click_sound.play(false, ctx->cfg.click_sound.volume);
		}
		if (st.right_pressed) {
			ctx->clicks.spawn(cx, cy, ctx->time_ms, true);
			if (ctx->cfg.click_sound.enabled && ctx->cfg.click_sound.right)
				ctx->click_sound.play(true, ctx->cfg.click_sound.volume);
		}
	}

	ctx->clicks.tick(ctx->time_ms);
	ctx->cursor.update(ctx->time_ms);
	ctx->zoom.update(ctx->time_ms);
	ctx->keys.tick(ctx->time_ms);
	ctx->highlight.update(ctx->time_ms);

	ctx->frame.cursor = ctx->cursor.render();
	ctx->spotlight.update(ctx->time_ms, ctx->frame.cursor.x, ctx->frame.cursor.y);
	ctx->frame.spotlight = ctx->spotlight.render();
	ctx->frame.zoom = ctx->zoom.render();
	ctx->frame.click_count = ctx->clicks.collect(ctx->time_ms, ctx->frame.clicks, kShaderClickSlots);
	ctx->frame.key_count =
		ctx->keys.collect(ctx->time_ms, ctx->frame.keys, FrameState::kMaxKeys);
	ctx->frame.highlight = ctx->highlight.render();
	ctx->frame.zoom_blur = ctx->cfg.motion_blur.enabled
				       ? core::clamp01(std::fabs(ctx->zoom.zoom_velocity()) * ctx->cfg.motion_blur.amount * 0.5)
				       : 0.0;

	/* Publishes state to the dock (UI thread). */
	ctx->effects_cache_.store(ctx->cfg.effects_enabled);
	ctx->preset_cache_.store(ctx->presets.current());
	ctx->profile_cache_.store(ctx->current_profile_);
	ctx->input_tier_cache_.store(ctx->input.tier());
}

void ts_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	auto *ctx = static_cast<ts_filter_ctx *>(data);

	if (!ctx->loaded) {
		obs_source_skip_video_filter(ctx->source);
		return;
	}

	/* The target source is only guaranteed valid inside the render callbacks. */
	obs_source_t *target = obs_filter_get_target(ctx->source);
	if (target) {
		ctx->target_w = obs_source_get_base_width(target);
		ctx->target_h = obs_source_get_base_height(target);
	}

	/* The capture's native cursor follows the master switch and the TopScreen
	 * cursor setting: hidden only while TopScreen actually draws its own. */
	ctx->sync_parent_cursor(ctx->cfg.effects_enabled && ctx->cfg.cursor.enabled);

	ctx->compositor.render(ctx->source, target, ctx->target_w, ctx->target_h, ctx->cfg,
			       ctx->frame);
}

void ts_filter_defaults(obs_data_t *settings)
{
	config_defaults(settings);
}

obs_properties_t *ts_filter_properties(void *data)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	obs_data_t *settings = ctx ? obs_source_get_settings(ctx->source) : nullptr;
	obs_properties_t *props = properties_create(data, settings);
	obs_data_release(settings);
	return props;
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
	ctx->export_profiles(path);
	return true;
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

	ctx->import_profiles(text);
	return true;
}

} // namespace topscreen

struct obs_source_info ts_filter_source_info = {
	.id = "topscreen_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = ts_filter_name,
	.create = ts_filter_create,
	.destroy = ts_filter_destroy,
	.get_defaults = ts_filter_defaults,
	.get_properties = ts_filter_properties,
	.update = ts_filter_update,
	.video_tick = ts_filter_tick,
	.video_render = ts_filter_render,
};
