#include "ts-capture-source.hpp"

#include "capture/ts-capture.hpp"

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
constexpr const char *kFilterFlag = "capture_effects_filter";
constexpr const char *kFilterId = "topscreen_filter";

struct CaptureSource {
	obs_source_t *source = nullptr;
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

	if (!ctx->texture || !effect)
		return;
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	if (image)
		gs_effect_set_texture(image, ctx->texture);
	gs_draw_sprite(ctx->texture, 0, 0, 0);
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
	start_backend(ctx);

	/* A single source that already carries the effects. The user can still
	 * open the filter to change every TopScreen setting. Keeping the filter as
	 * the effects owner means profiles, hotkeys and the dock keep working.
	 *
	 * OBS loads the saved filters AFTER create(). The flag prevents a duplicate
	 * filter on the next load; the filter itself is saved with the source. */
	const bool has_effects = obs_data_get_bool(settings, kFilterFlag);
	if (!has_effects) {
		obs_source_t *filter = obs_source_create(
			kFilterId, obs_module_text("FilterName"), nullptr, nullptr);
		if (filter) {
			obs_source_filter_add(source, filter);
			obs_source_release(filter);
			obs_data_set_bool(settings, kFilterFlag, true);
		} else {
			blog(LOG_WARNING,
			     "[TopScreen] could not add the effects filter to the capture source");
		}
	}
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
	if (ctx->backend)
		ctx->backend->set_capture_cursor(effective_cursor(ctx));
}

void capture_tick(void *data, float seconds)
{
	auto *ctx = static_cast<CaptureSource *>(data);
	if (!ctx)
		return;

	ctx->elapsed_ms += static_cast<double>(seconds) * 1000.0;
	++ctx->tick_serial;
	if (ctx->backend)
		ctx->backend->set_capture_cursor(effective_cursor(ctx));
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
	obs_data_set_default_int(settings, kMonitorKey, 0);
	obs_data_set_default_bool(settings, kCursorKey, true);
}

void find_effects_filter(obs_source_t *, obs_source_t *filter, void *param)
{
	if (!filter || !param)
		return;
	const char *id = obs_source_get_id(filter);
	if (id && std::strcmp(id, kFilterId) == 0)
		*static_cast<bool *>(param) = true;
}

bool has_effects_filter(obs_source_t *source)
{
	if (!source)
		return false;
	bool found = false;
	obs_source_enum_filters(source, find_effects_filter, &found);
	return found;
}

bool add_effects_clicked(obs_properties_t *props, obs_property_t *property, void *data)
{
	(void)props;
	(void)property;
	auto *ctx = static_cast<CaptureSource *>(data);
	if (!ctx || !ctx->source || has_effects_filter(ctx->source))
		return false;

	obs_source_t *filter = obs_source_create(kFilterId, obs_module_text("FilterName"),
						 nullptr, nullptr);
	if (!filter)
		return false;
	obs_source_filter_add(ctx->source, filter);
	obs_source_release(filter);

	obs_data_t *settings = obs_source_get_settings(ctx->source);
	if (settings) {
		obs_data_set_bool(settings, kFilterFlag, true);
		obs_source_update(ctx->source, settings);
		obs_data_release(settings);
	}
	return true;
}

obs_properties_t *capture_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	auto *ctx = static_cast<CaptureSource *>(data);
	obs_data_t *settings = ctx && ctx->source ? obs_source_get_settings(ctx->source) : nullptr;
	const int current = settings ? static_cast<int>(obs_data_get_int(settings, kMonitorKey)) : 0;

	obs_property_t *monitor = obs_properties_add_list(
		props, kMonitorKey, obs_module_text("Capture.Monitor"), OBS_COMBO_TYPE_LIST,
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
	(void)current;

	obs_properties_add_bool(props, kCursorKey, obs_module_text("Capture.Cursor"));

	if (ctx && ctx->source) {
		obs_properties_add_text(props, "capture_effects_hint",
					obs_module_text("Capture.EffectsHint"), OBS_TEXT_INFO);
		if (!has_effects_filter(ctx->source)) {
			obs_properties_add_button2(props, "capture_add_effects",
						   obs_module_text("Capture.AddEffects"),
						   add_effects_clicked, ctx);
		} else {
			obs_properties_add_text(props, "capture_effects_active",
						obs_module_text("Capture.EffectsActive"),
						OBS_TEXT_INFO);
		}
	}
	if (ctx && !ctx->logged_creation) {
		ctx->logged_creation = true;
		blog(LOG_INFO, "[TopScreen] capture source properties opened");
	}
	if (settings)
		obs_data_release(settings);
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
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = capture_name,
	.create = capture_create,
	.destroy = capture_destroy,
	.get_width = capture_width,
	.get_height = capture_height,
	.get_defaults = capture_defaults,
	.get_properties = capture_properties,
	.update = capture_update,
	.video_tick = capture_tick,
	.video_render = capture_render,
};

} // namespace topscreen
