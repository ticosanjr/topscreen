#include "ts-engine-internal.hpp"

namespace topscreen::engine_detail {

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
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(data);

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
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(data);
	if (!ctx)
		return;
	/* Does not touch cfg here (UI thread); the tick reloads. */
	ctx->pending_reload_.store(true);
}

void tick_engine_impl(void *data, float seconds)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(data);
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

} // namespace topscreen::engine_detail

namespace topscreen {

void *ts_engine_create(obs_data_t *settings, obs_source_t *source, bool source_mode)
{
	return engine_detail::create_engine_impl(settings, source, source_mode);
}

void ts_engine_destroy(void *engine)
{
	engine_detail::destroy_engine_impl(engine);
}

void ts_engine_update(void *engine, obs_data_t *settings)
{
	engine_detail::update_engine_impl(engine, settings);
}

void ts_engine_defaults(obs_data_t *settings)
{
	config_defaults(settings);
}

void ts_engine_set_target(void *engine, uint32_t width, uint32_t height)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(engine);
	if (!ctx)
		return;
	ctx->target_w = width;
	ctx->target_h = height;
}

void ts_engine_tick(void *engine, float seconds)
{
	engine_detail::tick_engine_impl(engine, seconds);
}

void ts_engine_render_filter(void *engine, gs_effect_t *effect)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(engine);
	if (ctx)
		ctx->render_filter(effect);
}

void ts_engine_render_texture(void *engine, gs_texture_t *texture, uint32_t width,
			      uint32_t height, gs_effect_t *effect)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(engine);
	if (ctx)
		ctx->render_texture(texture, width, height, effect);
}

obs_properties_t *ts_engine_properties(void *engine, obs_source_t *source,
				       void (*prefix)(obs_properties_t *, void *),
				       void *prefix_data)
{
	auto *ctx = static_cast<engine_detail::ts_filter_ctx *>(engine);
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
