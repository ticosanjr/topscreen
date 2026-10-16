#include "ts-filter.hpp"

#include "audio/ts-click-sound.hpp"
#include "core/coord.hpp"
#include "core/keys.hpp"
#include "hotkeys/ts-hotkeys.hpp"
#include "input/ts-input.hpp"
#include "render/ts-compositor.hpp"
#include "render/ts-frame.hpp"
#include "ts-presets.hpp"
#include "ts-properties.hpp"
#include "ts-registry.hpp"
#include "ts-settings.hpp"

#include <atomic>
#include <cmath>
#include <string>

using namespace topscreen;

namespace {

/* Acoes enfileiradas por hotkeys (thread de hotkey) e pelo dock (thread da UI).
 * Sao consumidas no video_tick, garantindo acesso single-thread aos modelos. */
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
};

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
 * Instancia do filtro TopScreen.
 *
 * Concorrencia:
 *  - cfg/modelos: somente a thread de video (tick/render) apos o create.
 *  - hotkeys/UI: enfileiram acoes (atomico); o tick aplica.
 *  - update(): apenas marca "reload"; o tick recarrega do obs_data.
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
	FrameState frame;

	uint32_t target_w = 0;
	uint32_t target_h = 0;
	double time_ms = 0.0;
	bool loaded = false;
	bool parent_cursor_checked = false;

	std::atomic<uint32_t> actions_{0};
	std::atomic<int> pending_effects_{-1};
	std::atomic<int> pending_preset_{-1};
	std::atomic<bool> pending_reload_{false};
	std::atomic<bool> effects_cache_{true};
	std::atomic<int> preset_cache_{-1};
	std::atomic<int> input_tier_cache_{0};

	void push_settings();
	void reconfigure_models();
	void request_preset(int index);
	void process_actions();
	void reload_from_settings();

	/* ---- HotkeyContext (thread de hotkey: apenas enfileira) ---- */
	void toggle_effects() override { actions_.fetch_or(A_TOGGLE_EFFECTS); }
	void toggle_background() override { actions_.fetch_or(A_TOGGLE_BG); }
	void zoom_in() override { actions_.fetch_or(A_ZOOM_IN); }
	void zoom_out() override { actions_.fetch_or(A_ZOOM_OUT); }
	void reset_zoom() override { actions_.fetch_or(A_RESET_ZOOM); }
	void next_preset() override { actions_.fetch_or(A_NEXT_PRESET); }
	void prev_preset() override { actions_.fetch_or(A_PREV_PRESET); }

	/* ---- FilterControl (thread da UI: apenas enfileira / le atomico) ---- */
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
	keys.configure(cfg.keys);
}

void ts_filter_ctx::request_preset(int index)
{
	if (index < 0)
		return;

	std::string err;
	if (!presets.apply(index, &cfg)) {
		blog(LOG_WARNING, "[TopScreen] preset invalido (%d)", index);
		return;
	}

	reconfigure_models();
	push_settings();
	blog(LOG_INFO, "[TopScreen] preset aplicado: %s", presets.name(index).c_str());
}

void ts_filter_ctx::reload_from_settings()
{
	obs_data_t *s = obs_source_get_settings(source);
	config_load(s, &cfg);
	obs_data_release(s);
	reconfigure_models();
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
	ctx->click_sound.load(ctx->cfg.click_sound.file.c_str());

	ctx->loaded = ctx->compositor.load();
	if (!ctx->loaded)
		blog(LOG_WARNING, "[TopScreen] falha ao carregar o efeito (passthrough)");

	ctx->cursor.reset(0.5, 0.5, 0.0);
	ctx->zoom.reset(0.0);

	ctx->effects_cache_.store(ctx->cfg.effects_enabled);

	ctx->hotkeys.register_all(source, ctx);
	ctx->input.start(source);

	registry_add(ctx);

	blog(LOG_INFO, "[TopScreen] filtro criado (presets=%d)", ctx->presets.count());

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
	/* Nao mexe no cfg aqui (thread da UI); o tick recarrega. */
	ctx->pending_reload_.store(true);
}

void ts_filter_tick(void *data, float seconds)
{
	auto *ctx = static_cast<ts_filter_ctx *>(data);
	ctx->time_ms += static_cast<double>(seconds) * 1000.0;

	if (ctx->pending_reload_.exchange(false))
		ctx->reload_from_settings();

	ctx->process_actions();

	ctx->input.poll();

	/* Teclas alimentam o overlay mesmo sem cursor disponivel. */
	for (int i = 0; i < ctx->input.state().key_count; ++i)
		ctx->keys.press(ctx->input.state().key_labels[i], ctx->input.state().key_modifier[i],
				ctx->time_ms);

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
	ctx->keys.tick(ctx->time_ms);

	ctx->frame.cursor = ctx->cursor.render();
	ctx->frame.zoom = ctx->zoom.render();
	ctx->frame.click_count = ctx->clicks.collect(ctx->time_ms, ctx->frame.clicks, kShaderClickSlots);
	ctx->frame.key_count =
		ctx->keys.collect(ctx->time_ms, ctx->frame.keys, FrameState::kMaxKeys);
	ctx->frame.zoom_blur = ctx->cfg.motion_blur.enabled
				       ? clamp01(std::fabs(ctx->zoom.zoom_velocity()) * ctx->cfg.motion_blur.amount * 0.5)
				       : 0.0;

	/* Publica estado para o dock (thread da UI). */
	ctx->effects_cache_.store(ctx->cfg.effects_enabled);
	ctx->preset_cache_.store(ctx->presets.current());
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
