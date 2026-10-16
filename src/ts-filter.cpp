#include "ts-filter.hpp"

#include "hotkeys/ts-hotkeys.hpp"
#include "input/ts-input.hpp"
#include "render/ts-compositor.hpp"
#include "ts-properties.hpp"
#include "ts-settings.hpp"

using namespace topscreen;

namespace {

/**
 * Estado de uma instancia do filtro TopScreen.
 *
 * Implementa HotkeyContext para que os atalhos registrados no OBS possam
 * alterar a configuracao em tempo real.
 */
struct ts_filter_ctx final : public HotkeyContext {
	obs_source_t *source = nullptr;
	Config cfg;
	Compositor compositor;
	Input input;
	Hotkeys hotkeys;
	bool loaded = false;

	/** Persiste a config atual de volta no obs_data da fonte. */
	void push_settings();

	/* ---- HotkeyContext ---- */
	void toggle_effects() override;
	void toggle_background() override;
	void zoom_in() override;
	void zoom_out() override;
	void reset_zoom() override;
	void next_preset() override;
	void prev_preset() override;
};

void ts_filter_ctx::push_settings()
{
	obs_data_t *s = obs_source_get_settings(source);
	obs_data_set_bool(s, key::kEffectsEnabled, cfg.effects_enabled);
	obs_data_set_bool(s, key::kBgEnabled, cfg.bg.enabled);
	obs_data_set_double(s, key::kZoom, cfg.zoom.zoom);
	obs_source_update(source, s);
	obs_data_release(s);
}

void ts_filter_ctx::toggle_effects()
{
	cfg.effects_enabled = !cfg.effects_enabled;
	push_settings();
}

void ts_filter_ctx::toggle_background()
{
	cfg.bg.enabled = !cfg.bg.enabled;
	push_settings();
}

void ts_filter_ctx::zoom_in()
{
	cfg.zoom.zoom = cfg.zoom.zoom * 1.25;
	if (cfg.zoom.zoom > 4.0)
		cfg.zoom.zoom = 4.0;
	push_settings();
}

void ts_filter_ctx::zoom_out()
{
	cfg.zoom.zoom = cfg.zoom.zoom / 1.25;
	if (cfg.zoom.zoom < 1.0)
		cfg.zoom.zoom = 1.0;
	push_settings();
}

void ts_filter_ctx::reset_zoom()
{
	cfg.zoom.zoom = 1.0;
	push_settings();
}

void ts_filter_ctx::next_preset()
{
	/* TODO(F3.7): ciclar presets de data/presets. */
	blog(LOG_DEBUG, "[TopScreen] next_preset (nao implementado)");
}

void ts_filter_ctx::prev_preset()
{
	/* TODO(F3.7): ciclar presets de data/presets. */
	blog(LOG_DEBUG, "[TopScreen] prev_preset (nao implementado)");
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

	ctx->loaded = ctx->compositor.load();
	if (!ctx->loaded)
		blog(LOG_WARNING, "[TopScreen] falha ao carregar o efeito (passthrough)");

	ctx->hotkeys.register_all(source, ctx);
	ctx->input.start(source);

	return ctx;
}

void ts_filter_destroy(void *data)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);

	ctx->input.stop();
	ctx->hotkeys.unregister_all();
	ctx->compositor.release();

	delete ctx;
}

void ts_filter_update(void *data, obs_data_t *settings)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	config_load(settings, &ctx->cfg);
}

void ts_filter_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	ctx->input.poll();
}

void ts_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	auto *ctx = static_cast<ts_filter_ctx *>(data);

	if (!ctx->loaded) {
		obs_source_skip_video_filter(ctx->source);
		return;
	}

	ctx->compositor.render(ctx->source, ctx->cfg);
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
