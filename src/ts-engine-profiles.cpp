#include "ts-engine-internal.hpp"

namespace topscreen::engine_detail {

std::string config_fingerprint(const core::Config &cfg)
{
	obs_data_t *s = obs_data_create();
	config_store(s, cfg);
	const char *json = obs_data_get_json(s);
	std::string out = json ? json : "";
	obs_data_release(s);
	return out;
}

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

} // namespace topscreen::engine_detail

namespace topscreen {

using namespace core::keys;

bool profiles_export_from_properties(void *filter)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(filter);
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
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(filter);
	if (!ctx || !ctx->source)
		return false;
	ctx->reset_profiles();
	return true;
}

bool profiles_rename_from_properties(void *filter)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(filter);
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
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(filter);
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
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(filter);
	if (!ctx)
		return false;
	ctx->revert_profile();
	return true;
}

bool profiles_overwrite_confirmed(void *filter)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(filter);
	if (!ctx || !ctx->source)
		return false;
	obs_data_t *s = obs_source_get_settings(ctx->source);
	const bool confirmed = s && obs_data_get_bool(s, kProfileConfirmOverwrite);
	obs_data_release(s);
	return confirmed;
}

bool profiles_import_from_properties(void *filter)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(filter);
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
