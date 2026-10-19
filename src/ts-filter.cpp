#include "ts-filter.hpp"

#include "audio/ts-click-sound.hpp"
#include "core/coord.hpp"
#include "core/keys.hpp"
#include "core/math.hpp"
#include "core/profiles.hpp"
#include "hotkeys/ts-hotkeys.hpp"
#include "input/ts-input.hpp"
#include "input/ts-monitor.hpp"
#include "render/ts-compositor.hpp"
#include "render/ts-frame.hpp"
#include "ts-presets.hpp"
#include "ts-properties.hpp"
#include "ts-registry.hpp"
#include "ts-settings.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
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
};

/**
 * Disables the parent source's native cursor (e.g.: Display Capture).
 *
 * Different sources use different names for the same option:
 *   - "capture_cursor": Windows (monitor_capture) and some backends
 *   - "show_cursor":    Linux (xshm_input), macOS (screen_capture)
 *
 * We do this so we can draw our own cursor without duplicating it.
 */
void disable_parent_cursor(obs_source_t *parent)
{
	if (!parent)
		return;

	obs_properties_t *props = obs_source_properties(parent);
	if (!props)
		return;

	static const char *const kCursorKeys[] = {"capture_cursor", "show_cursor"};

	bool found = false;
	for (const char *key : kCursorKeys) {
		if (!obs_properties_get(props, key))
			continue;

		obs_data_t *s = obs_source_get_settings(parent);
		if (!s)
			continue;

		const bool was = obs_data_get_bool(s, key);
		obs_data_set_bool(s, key, false);
		obs_source_update(parent, s);
		obs_data_release(s);

		blog(LOG_INFO, "[TopScreen] capture native cursor disabled (%s, was %s)", key,
		     was ? "on" : "off");
		found = true;
	}

	if (!found)
		blog(LOG_INFO, "[TopScreen] parent source does not expose a native cursor option (%s)",
		     parent ? obs_source_get_name(parent) : "?");

	obs_properties_destroy(props);
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
	FrameState frame;

	uint32_t target_w = 0;
	uint32_t target_h = 0;
	double time_ms = 0.0;
	bool loaded = false;
	bool parent_cursor_checked = false;
	bool prev_left = false;

	int current_profile_ = 0;
	int last_profile = -1;

	/* Origin of the captured monitor in GLOBAL desktop coordinates: the input
	 * reports global positions, so a monitor below/left of the primary one
	 * needs its offset subtracted before normalizing. */
	double origin_x_ = 0.0;
	double origin_y_ = 0.0;
	bool origin_known_ = false;
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

	void push_settings();
	void reconfigure_models();
	void request_preset(int index);
	void apply_profile(int index);
	void store_profile_slot(int index);
	void do_save_profile();
	void process_actions();
	void reload_from_settings();
	void refresh_monitor_origin(bool force);

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

	/* ---- FilterControl (UI thread: only queues / reads atomic) ---- */
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
		*x = origin_known_ ? origin_x_ : 0.0;
		*y = origin_known_ ? origin_y_ : 0.0;
	}

	/* ---- FilterControl: profiles ---- */
	int profile_count() const override { return kProfileCount; }
	int current_profile() const override { return profile_cache_.load(); }
};

void ts_filter_ctx::push_settings()
{
	obs_data_t *s = obs_source_get_settings(source);
	config_store(s, cfg);
	obs_data_set_int(s, kProfileCurrent, current_profile_);
	obs_source_update(source, s);
	obs_data_release(s);
}

void ts_filter_ctx::apply_profile(int index)
{
	if (index < 0 || index >= kProfileCount)
		return;

	obs_data_t *s = obs_source_get_settings(source);
	obs_data_t *p = obs_data_get_obj(s, kProfileSlots[index]);
	if (p) {
		config_load(p, &cfg);
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

void ts_filter_ctx::do_save_profile()
{
	store_profile_slot(current_profile_);
	blog(LOG_INFO, "[TopScreen] profile saved: %s", core::profile_label(current_profile_));
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

	const bool known = ts_resolve_monitor_origin(device.c_str(), hint, trust_hint, target_w,
						     target_h, &origin_x_, &origin_y_);
	if (!known) {
		origin_x_ = 0.0;
		origin_y_ = 0.0;
	}
	origin_known_ = known;
}

void ts_filter_ctx::reconfigure_models()
{
	cursor.configure(cfg.cursor);
	zoom.configure(cfg.zoom);
	clicks.set_left(cfg.click_left);
	clicks.set_right(cfg.click_right);
	keys.configure(cfg.keys);
	highlight.configure(cfg.highlight);
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
	push_settings();
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
		 * syncs the main keys. Without the push, the next reload
		 * (any other property) would reload the old keys and
		 * undo the profile.
		 */
		last_profile = static_cast<int>(pc);
		current_profile_ = want;
		apply_profile(current_profile_);
		reconfigure_models();
		push_settings();
		blog(LOG_INFO, "[TopScreen] profile activated: %s", core::profile_label(current_profile_));
		return;
	}

	/*
	 * Normal edit: the active profile follows what is on screen (auto-save),
	 * so the user's changes are not lost when switching profiles.
	 */
	current_profile_ = want;
	reconfigure_models();
	/* Reloads the click sounds when the selection changed (slots keep the
	 * unchanged side, so this is cheap). */
	click_sound.load(cfg.click_sound.file_left.c_str(), cfg.click_sound.file_right.c_str());
	store_profile_slot(current_profile_);
}

void ts_filter_ctx::process_actions()
{
	const uint32_t act = actions_.exchange(0);
	if (act == 0)
		return;

	if (act & A_TOGGLE_EFFECTS) {
		cfg.effects_enabled = !cfg.effects_enabled;
		push_settings();
	}
	if (act & A_SET_EFFECTS) {
		const int v = pending_effects_.exchange(-1);
		if (v >= 0) {
			cfg.effects_enabled = (v != 0);
			push_settings();
		}
	}
	if (act & A_TOGGLE_BG) {
		cfg.bg.enabled = !cfg.bg.enabled;
		push_settings();
	}
	if (act & A_ZOOM_IN)
		zoom.manual_step(1.25);
	if (act & A_ZOOM_OUT)
		zoom.manual_step(0.8);
	if (act & A_RESET_ZOOM)
		zoom.set_manual(1.0);
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
			current_profile_ = i;
			last_profile = i;
			apply_profile(i);
			reconfigure_models();
			push_settings();
			blog(LOG_INFO, "[TopScreen] profile activated: %s", core::profile_label(i));
		}
	}
	if (act & A_SAVE_PROFILE)
		do_save_profile();
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

	config_load(settings, &ctx->cfg);

	const long long pc = obs_data_get_int(settings, kProfileCurrent);
	ctx->current_profile_ = std::max(0, std::min(kProfileCount - 1, static_cast<int>(pc)));
	ctx->last_profile = ctx->current_profile_;
	ctx->profile_cache_.store(ctx->current_profile_);

	/* Guarantees the 4 profile slots (seeds visible variations into the empty ones). */
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

	ctx->hotkeys.register_all(source, ctx);
	ctx->input.start(source);

	registry_add(ctx);

	blog(LOG_INFO, "[TopScreen] filter created (presets=%d)", ctx->presets.count());

	return ctx;
}

void ts_filter_destroy(void *data)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);

	registry_remove(ctx);
	ctx->input.stop();
	ctx->hotkeys.unregister_all();
	ctx->click_sound.release();
	ctx->compositor.release();

	delete ctx;
}

void ts_filter_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
	auto *ctx = static_cast<ts_filter_ctx *>(data);
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
		const core::Monitor mon{ctx->origin_known_ ? ctx->origin_x_ : 0.0,
					ctx->origin_known_ ? ctx->origin_y_ : 0.0,
					static_cast<double>(ctx->target_w),
					static_cast<double>(ctx->target_h)};
		const core::Vec2f c = core::screen_to_content(st.x, st.y, mon);

		const double cx = core::clamp01(c.x);
		const double cy = core::clamp01(c.y);

		ctx->cursor.set_target(cx, cy, ctx->time_ms);
		ctx->zoom.set_cursor(cx, cy, ctx->time_ms);

		/* Drag highlighter (left button). */
		if (st.left_pressed)
			ctx->highlight.begin(cx, cy, ctx->time_ms);
		else if (st.left)
			ctx->highlight.move(cx, cy, ctx->time_ms);
		if (!st.left && ctx->prev_left)
			ctx->highlight.end(ctx->time_ms);
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

	/* Disables the parent source's native cursor only once (inside render). */
	if (!ctx->parent_cursor_checked) {
		ctx->parent_cursor_checked = true;
		disable_parent_cursor(obs_filter_get_parent(ctx->source));
	}

	ctx->compositor.render(ctx->source, ctx->cfg, ctx->frame);
}

void ts_filter_defaults(obs_data_t *settings)
{
	config_defaults(settings);
}

obs_properties_t *ts_filter_properties(void *data)
{
	return properties_create(data);
}

} // namespace

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
