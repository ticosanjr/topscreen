#include "ts-capture-source.hpp"

#include "capture/ts-capture.hpp"
#include "audio/ts-click-source.hpp"
#include "ts-engine.hpp"
#include "core/config.hpp"
#include "core/keys.hpp"
#include "ts-profile-store.hpp"
#include "ts-settings.hpp"

#include <obs-module.h>

#include <graphics/graphics.h>

#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace topscreen {

namespace {

constexpr const char *kMonitorKey = "capture_monitor";
constexpr const char *kCursorKey = "capture_cursor";
constexpr const char *kClickAudioKey = "capture_click_audio";
constexpr uint32_t kDefaultMixers = 0x3F;
constexpr const char *kFilterId = "topscreen_filter";

struct CaptureSource {
	obs_source_t *source = nullptr;
	void *engine = nullptr;
	bool migrated_attached_filter = false;
	std::unique_ptr<capture::Backend> backend;
	std::atomic<int> pending_monitor{-1};
	std::atomic<int> capture_failures{0};
	std::atomic<bool> restart_requested{false};
	int monitor = 0;
	bool started = false;
	bool logged_creation = false;
	double elapsed_ms = 0.0;
	double retry_at_ms = 0.0;

	gs_texture_t *texture = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	capture::PixelFormat format = capture::PixelFormat::Bgra;
	/* Read by obs_source_get_width() (possibly UI) while the video thread
	 * updates the texture. */
	std::atomic<uint32_t> display_width{0};
	std::atomic<uint32_t> display_height{0};
	std::atomic<bool> capture_cursor{true};
	std::atomic<bool> runtime_cursor_hidden{false};
	std::atomic<bool> click_audio{false};
	uint64_t audio_next_ts = 0;
	uint64_t tick_serial = 0;
	uint64_t captured_serial = 0;
};

std::mutex &source_mutex()
{
	static std::mutex m;
	return m;
}

std::unordered_map<obs_source_t *, CaptureSource *> &source_map()
{
	static std::unordered_map<obs_source_t *, CaptureSource *> map;
	return map;
}

bool effective_cursor(const CaptureSource *ctx)
{
	return ctx && ctx->capture_cursor.load() && !ctx->runtime_cursor_hidden.load();
}

void migrate_attached_filter(CaptureSource *ctx);

void start_backend(CaptureSource *ctx)
{
	if (!ctx || !ctx->backend)
		return;

	capture::Config config;
	config.monitor = ctx->monitor;
	config.capture_cursor = effective_cursor(ctx);
	ctx->captured_serial = 0;
	ctx->started = ctx->backend->start(config);
	if (ctx->started) {
		ctx->capture_failures.store(0);
		ctx->backend->size(&ctx->width, &ctx->height);
		ctx->display_width.store(ctx->width);
		ctx->display_height.store(ctx->height);
		blog(LOG_INFO, "[TopScreen] capture started (%s, monitor %d, %ux%u)",
		     ctx->backend->id(), ctx->monitor, ctx->width, ctx->height);
	} else {
		ctx->width = 0;
		ctx->height = 0;
		ctx->display_width.store(0);
		ctx->display_height.store(0);
		blog(LOG_WARNING, "[TopScreen] capture backend could not start (%s)",
		     ctx->backend ? ctx->backend->id() : "none");
	}
}

void release_texture(CaptureSource *ctx)
{
	if (ctx && ctx->texture) {
		gs_texture_destroy(ctx->texture);
		ctx->texture = nullptr;
	}
}

void render_frame(CaptureSource *ctx, gs_effect_t *effect)
{
	if (!ctx || !ctx->started || !ctx->backend)
		return;

	capture::Frame frame;
	bool fresh = false;
	/* OBS can render the same source more than once per tick (preview and
	 * program). Capture at most once per tick and reuse the texture. */
	if (ctx->captured_serial != ctx->tick_serial) {
		fresh = ctx->backend->capture(&frame) && frame.data && frame.width &&
			frame.height;
		ctx->captured_serial = ctx->tick_serial;
	}
	if (fresh) {
		ctx->capture_failures.store(0);
		const gs_color_format color_format =
			frame.format == capture::PixelFormat::Rgba ? GS_RGBA : GS_BGRA;

		if (!ctx->texture || ctx->width != frame.width ||
		    ctx->height != frame.height || ctx->format != frame.format) {
			release_texture(ctx);
			const uint8_t *planes[1] = {frame.data};
			ctx->texture = gs_texture_create(frame.width, frame.height,
							 color_format, 1, planes,
							 GS_DYNAMIC);
			ctx->width = frame.width;
			ctx->height = frame.height;
			ctx->format = frame.format;
			ctx->display_width.store(frame.width);
			ctx->display_height.store(frame.height);
			if (!ctx->texture) {
				blog(LOG_WARNING,
				     "[TopScreen] could not create the capture texture");
				return;
			}
		} else {
			gs_texture_set_image(ctx->texture, frame.data, frame.stride, false);
		}
	} else if (ctx->capture_failures.fetch_add(1) >= 10) {
		/* A monitor unplug or a server reset can make XShmGetImage fail
		 * forever. Ask the video thread to reopen the backend. */
		ctx->restart_requested.store(true);
	}

	if (!ctx->texture || !ctx->engine)
		return;
	ts_engine_render_texture(ctx->engine, ctx->texture, ctx->width, ctx->height, effect);
}

const char *capture_name(void *)
{
	return obs_module_text("Capture.Name");
}

void *capture_create(obs_data_t *settings, obs_source_t *source)
{
	auto *ctx = new CaptureSource();
	ctx->source = source;
	{
		std::lock_guard<std::mutex> lock(source_mutex());
		source_map()[source] = ctx;
	}
	ctx->backend = capture::create_backend();
	ctx->monitor = static_cast<int>(obs_data_get_int(settings, kMonitorKey));
	if (ctx->monitor < 0)
		ctx->monitor = 0;
	ctx->capture_cursor.store(obs_data_get_bool(settings, kCursorKey));
	ctx->click_audio.store(obs_data_get_bool(settings, kClickAudioKey));
	obs_source_set_audio_mixers(source, kDefaultMixers);

	/* Effects are applied by the shared engine; there is no child filter. */
	ctx->engine = ts_engine_create(settings, source, true);
	start_backend(ctx);
	return ctx;
}

void capture_destroy(void *data)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	if (!ctx)
		return;
	{
		std::lock_guard<std::mutex> lock(source_mutex());
		source_map().erase(ctx->source);
	}
	if (ctx->texture) {
		obs_enter_graphics();
		release_texture(ctx);
		obs_leave_graphics();
	}
	if (ctx->engine)
		ts_engine_destroy(ctx->engine);
	if (ctx->backend)
		ctx->backend->stop();
	delete ctx;
}

void capture_update(void *data, obs_data_t *settings)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	if (!ctx)
		return;
	const int monitor = static_cast<int>(obs_data_get_int(settings, kMonitorKey));
	if (monitor >= 0 && monitor != ctx->monitor)
		ctx->pending_monitor.store(monitor);
	const bool cursor = obs_data_get_bool(settings, kCursorKey);
	ctx->capture_cursor.store(cursor);
	ctx->click_audio.store(obs_data_get_bool(settings, kClickAudioKey));
	if (ctx->backend)
		ctx->backend->set_capture_cursor(effective_cursor(ctx));
	if (ctx->engine)
		ts_engine_update(ctx->engine, settings);
}

bool capture_audio_render(void *data, uint64_t *ts_out, struct obs_source_audio_mix *audio,
			  uint32_t mixers, size_t channels, size_t sample_rate)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	if (!ctx || !ctx->click_audio.load())
		return false;
	return ts_click_audio_mix(&ctx->audio_next_ts, ts_out, audio, mixers, channels,
				  sample_rate);
}

void capture_activate(void *data)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	if (ctx)
		ctx->audio_next_ts = 0;
}

void capture_tick(void *data, float seconds)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	if (!ctx)
		return;

	ctx->elapsed_ms += static_cast<double>(seconds) * 1000.0;
	++ctx->tick_serial;
	migrate_attached_filter(ctx);
	if (ctx->backend)
		ctx->backend->set_capture_cursor(effective_cursor(ctx));
	uint32_t engine_w = 0, engine_h = 0;
	if (ctx->backend)
		ctx->backend->size(&engine_w, &engine_h);
	if (ctx->engine) {
		ts_engine_set_target(ctx->engine, engine_w, engine_h);
		ts_engine_tick(ctx->engine, seconds);
	}
	const int pending = ctx->pending_monitor.exchange(-1);
	if (pending >= 0 && pending != ctx->monitor) {
		ctx->monitor = pending;
		if (ctx->backend)
			ctx->backend->stop();
		start_backend(ctx);
	}
	if (ctx->restart_requested.exchange(false)) {
		if (ctx->backend)
			ctx->backend->stop();
		start_backend(ctx);
	}
	if (!ctx->started && ctx->elapsed_ms >= ctx->retry_at_ms) {
		ctx->retry_at_ms = ctx->elapsed_ms + 2000.0;
		start_backend(ctx);
	}
}

void capture_render(void *data, gs_effect_t *effect)
{
	render_frame(static_cast<CaptureSource *>(data), effect);
}

uint32_t capture_width(void *data)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	return ctx ? ctx->display_width.load() : 0;
}

uint32_t capture_height(void *data)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	return ctx ? ctx->display_height.load() : 0;
}

void capture_defaults(obs_data_t *settings)
{
	ts_engine_defaults(settings);
	obs_data_set_default_int(settings, kMonitorKey, 0);
	obs_data_set_default_bool(settings, kCursorKey, true);
	obs_data_set_default_bool(settings, kClickAudioKey, false);
}

void migrate_attached_filter(CaptureSource *ctx)
{
	if (!ctx || ctx->migrated_attached_filter || !ctx->source || !ctx->engine)
		return;
	ctx->migrated_attached_filter = true;

	struct Found {
		obs_source_t *filter = nullptr;
	};
	Found found;
	obs_source_enum_filters(
		ctx->source,
		[](obs_source_t *, obs_source_t *filter, void *param) {
			auto *out = static_cast<Found *>(param);
			const char *id = obs_source_get_id(filter);
			if (id && std::strcmp(id, kFilterId) == 0 && !out->filter)
				out->filter = obs_source_get_ref(filter);
		},
		&found);
	if (!found.filter)
		return;

	/* 1.2.5 attached a real TopScreen FX filter to this source. Move its
	 * settings/profiles into the source and remove the child filter so the
	 * embedded engine does not process the effects twice. */
	obs_data_t *filter_settings = obs_source_get_settings(found.filter);
	obs_data_t *source_settings = obs_source_get_settings(ctx->source);
	if (filter_settings && source_settings) {
		core::Config cfg;
		config_load(filter_settings, &cfg);
		config_store(source_settings, cfg);
		obs_data_set_int(source_settings, core::keys::kProfileCurrent,
				 obs_data_get_int(filter_settings, core::keys::kProfileCurrent));
		obs_data_set_int(source_settings, core::keys::kProfileSeedVersion,
				 obs_data_get_int(filter_settings, core::keys::kProfileSeedVersion));
		for (int i = 0; i < core::keys::kProfileCount; ++i) {
			profile_store::store_slot(
				source_settings, i,
				profile_store::load_slot(filter_settings, i));
		}
		obs_source_update(ctx->source, source_settings);
		ts_engine_update(ctx->engine, source_settings);
	}
	obs_data_release(filter_settings);
	obs_data_release(source_settings);
	obs_source_filter_remove(ctx->source, found.filter);
	obs_source_release(found.filter);
	blog(LOG_INFO, "[TopScreen] migrated attached effects filter into TopScreen Capture");
}

obs_properties_t *capture_properties(void *data)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	obs_source_t *source = ctx ? ctx->source : nullptr;
	obs_properties_t *props = ctx && ctx->engine
		? ts_engine_properties(ctx->engine, source)
		: obs_properties_create();

	obs_properties_t *capture = obs_properties_create();
	obs_data_t *settings = source ? obs_source_get_settings(source) : nullptr;
	obs_property_t *monitor = obs_properties_add_list(
		capture, kMonitorKey, obs_module_text("Capture.Monitor"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	const auto monitors = capture::enumerate_monitors();
	if (monitors.empty()) {
		obs_property_list_add_int(monitor, obs_module_text("Capture.MonitorUnavailable"),
					  0);
	} else {
		for (const auto &info : monitors) {
			const std::string label = info.name + " — " + std::to_string(info.width) +
						  "x" + std::to_string(info.height);
			obs_property_list_add_int(monitor, label.c_str(), info.index);
		}
	}
	obs_properties_add_bool(capture, kCursorKey, obs_module_text("Capture.Cursor"));
	obs_properties_add_bool(capture, kClickAudioKey, obs_module_text("Capture.ClickAudio"));
	obs_properties_add_text(capture, "capture_effects_hint",
				obs_module_text("Capture.EffectsHint"), OBS_TEXT_INFO);
	obs_properties_add_group(props, "grp_capture_source",
				 obs_module_text("Capture.SourceGroup"), OBS_GROUP_NORMAL,
				 capture);
	if (settings)
		obs_data_release(settings);

	if (ctx && !ctx->logged_creation) {
		ctx->logged_creation = true;
		blog(LOG_INFO, "[TopScreen] capture source properties opened");
	}
	return props;
}

} // namespace

void set_native_cursor(obs_source_t *source, bool enabled)
{
	std::lock_guard<std::mutex> lock(source_mutex());
	const auto it = source_map().find(source);
	if (it == source_map().end())
		return;
	const bool was_hidden = it->second->runtime_cursor_hidden.exchange(!enabled);
	if (was_hidden != !enabled)
		blog(LOG_INFO, "[TopScreen] capture native cursor %s (runtime)",
		     enabled ? "enabled" : "disabled");
	if (it->second->backend)
		it->second->backend->set_capture_cursor(effective_cursor(it->second));
}

bool native_cursor_enabled(obs_source_t *source)
{
	std::lock_guard<std::mutex> lock(source_mutex());
	const auto it = source_map().find(source);
	return it != source_map().end() && effective_cursor(it->second);
}

struct obs_source_info ts_capture_source_info = {
	.id = "topscreen_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_AUDIO,
	.get_name = capture_name,
	.create = capture_create,
	.destroy = capture_destroy,
	.get_width = capture_width,
	.get_height = capture_height,
	.get_defaults = capture_defaults,
	.get_properties = capture_properties,
	.update = capture_update,
	.activate = capture_activate,
	.video_tick = capture_tick,
	.video_render = capture_render,
	.audio_render = capture_audio_render,
};

} // namespace topscreen
