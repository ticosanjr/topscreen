#include "ts-filter.hpp"

#include "audio/ts-click-sound.hpp"
#include "core/coord.hpp"
#include "hotkeys/ts-hotkeys.hpp"
#include "input/ts-input.hpp"
#include "render/ts-compositor.hpp"
#include "render/ts-frame.hpp"
#include "ts-presets.hpp"
#include "ts-properties.hpp"
#include "ts-settings.hpp"

using namespace topscreen;

namespace {

double clamp01(double v)
{
	return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
}

/**
 * Desativa o cursor nativo da fonte-pai (ex.: Captura de Tela).
 *
 * Fazemos isso para poder desenhar o nosso cursor sem duplicar. Só age quando
 * a fonte expõe a propriedade "capture_cursor".
 */
void disable_parent_cursor(obs_source_t *parent)
{
	if (!parent)
		return;

	obs_properties_t *props = obs_source_properties(parent);
	if (!props)
		return;

	if (obs_properties_get(props, "capture_cursor")) {
		obs_data_t *s = obs_source_get_settings(parent);
		if (s) {
			if (obs_data_get_bool(s, "capture_cursor")) {
				obs_data_set_bool(s, "capture_cursor", false);
				obs_source_update(parent, s);
				blog(LOG_INFO, "[TopScreen] cursor nativo da captura desativado");
			}
			obs_data_release(s);
		}
	}

	obs_properties_destroy(props);
}

/**
 * Estado de uma instancia do filtro TopScreen.
 *
 * Implementa HotkeyContext para que os atalhos registrados no OBS alterem a
 * configuracao em tempo real. Mantem os modelos de dynamics/effects do nucleo
 * e monta o FrameState consumido pelo compositor.
 */
struct ts_filter_ctx final : public HotkeyContext {
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
	FrameState frame;

	uint32_t target_w = 0;
	uint32_t target_h = 0;
	double time_ms = 0.0;
	bool loaded = false;
	bool parent_cursor_checked = false;

	void push_settings();
	void reconfigure_models();
	void apply_preset(int index);

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
	config_store(s, cfg);
	obs_source_update(source, s);
	obs_data_release(s);
}

void ts_filter_ctx::reconfigure_models()
{
	cursor.configure(cfg.cursor);
	zoom.configure(cfg.zoom);
	clicks.set_left(cfg.click_left);
	clicks.set_right(cfg.click_right);
}

void ts_filter_ctx::apply_preset(int index)
{
	if (index < 0)
		return;
	if (!presets.apply(index, &cfg))
		return;
	reconfigure_models();
	push_settings();
	blog(LOG_INFO, "[TopScreen] preset aplicado: %s", presets.name(index).c_str());
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
	zoom.manual_step(1.25);
}

void ts_filter_ctx::zoom_out()
{
	zoom.manual_step(0.8);
}

void ts_filter_ctx::reset_zoom()
{
	zoom.set_manual(1.0);
}

void ts_filter_ctx::next_preset()
{
	apply_preset(presets.next());
}

void ts_filter_ctx::prev_preset()
{
	apply_preset(presets.prev());
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
	ctx->reconfigure_models();

	ctx->presets.load();

	ctx->loaded = ctx->compositor.load();
	if (!ctx->loaded)
		blog(LOG_WARNING, "[TopScreen] falha ao carregar o efeito (passthrough)");

	ctx->cursor.reset(0.5, 0.5, 0.0);
	ctx->zoom.reset(0.0);

	ctx->hotkeys.register_all(source, ctx);
	ctx->input.start(source);

	blog(LOG_INFO, "[TopScreen] filtro criado (presets=%d, input tier=%d)", ctx->presets.count(),
	     ctx->input.tier());

	return ctx;
}

void ts_filter_destroy(void *data)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);

	ctx->input.stop();
	ctx->hotkeys.unregister_all();
	ctx->click_sound.release();
	ctx->compositor.release();

	delete ctx;
}

void ts_filter_update(void *data, obs_data_t *settings)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	config_load(settings, &ctx->cfg);
	ctx->reconfigure_models();
}

void ts_filter_tick(void *data, float seconds)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	ctx->time_ms += static_cast<double>(seconds) * 1000.0;

	ctx->input.poll();

	if (ctx->input.available() && ctx->target_w > 0 && ctx->target_h > 0) {
		const InputState &st = ctx->input.state();
		const core::Monitor mon{0.0, 0.0, static_cast<double>(ctx->target_w),
					static_cast<double>(ctx->target_h)};
		const core::Vec2f c = core::screen_to_content(st.x, st.y, mon);

		const double cx = clamp01(c.x);
		const double cy = clamp01(c.y);

		ctx->cursor.set_target(cx, cy, ctx->time_ms);
		ctx->zoom.set_cursor(cx, cy, ctx->time_ms);

		if (st.left_pressed) {
			ctx->clicks.spawn(cx, cy, ctx->time_ms, false);
			if (ctx->cfg.click_sound.enabled && ctx->cfg.click_sound.left)
				ctx->click_sound.play();
		}
		if (st.right_pressed) {
			ctx->clicks.spawn(cx, cy, ctx->time_ms, true);
			if (ctx->cfg.click_sound.enabled && ctx->cfg.click_sound.right)
				ctx->click_sound.play();
		}
	}

	ctx->clicks.tick(ctx->time_ms);
	ctx->cursor.update(ctx->time_ms);
	ctx->zoom.update(ctx->time_ms);

	ctx->frame.cursor = ctx->cursor.render();
	ctx->frame.zoom = ctx->zoom.render();
	ctx->frame.click_count = ctx->clicks.collect(ctx->time_ms, ctx->frame.clicks, kShaderClickSlots);
}

void ts_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	auto *ctx = static_cast<ts_filter_ctx *>(data);

	if (!ctx->loaded) {
		obs_source_skip_video_filter(ctx->source);
		return;
	}

	/* A fonte-alvo só é garantidamente válida dentro dos callbacks de render. */
	obs_source_t *target = obs_filter_get_target(ctx->source);
	if (target) {
		ctx->target_w = obs_source_get_base_width(target);
		ctx->target_h = obs_source_get_base_height(target);
	}

	/* T2.1: uma unica vez, desativa o cursor nativo da fonte-pai. */
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
