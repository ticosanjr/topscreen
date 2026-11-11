#include "ts-webcam-source.hpp"

#include "core/keys.hpp"
#include "core/webcam.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <string>

namespace topscreen::webcam_source {

namespace {

constexpr const char *kSourceName = "TopScreen Webcam";
constexpr const char *kFilterId = "topscreen_webcam_bubble";
constexpr const char *kFilterName = "Webcam Bubble";

const char *camera_input_id()
{
#if defined(_WIN32)
	return "dshow_input";
#elif defined(__linux__)
	return "v4l2_input";
#else
	return nullptr;
#endif
}

std::string unique_source_name(obs_scene_t *scene)
{
	std::string name = kSourceName;
	for (int suffix = 2; obs_scene_find_source(scene, name.c_str()); ++suffix) {
		name = std::string(kSourceName) + " " + std::to_string(suffix);
		if (suffix > 99)
			break;
	}
	return name;
}

bool ensure_bubble_filter(obs_source_t *source)
{
	if (!source)
		return false;

	obs_source_t *existing = obs_source_get_filter_by_name(source, kFilterName);
	if (existing) {
		obs_source_release(existing);
		return true;
	}

	/* The scene item is already the bubble rectangle, so the filter works on
	 * the full source and the user can drag/resize the camera source normally.
	 * This is why the mask size is 1.0 and the anchor is centered here. */
	obs_data_t *settings = obs_data_create();
	obs_data_set_double(settings, core::keys::kWebcamSize, 1.0);
	obs_data_set_int(settings, core::keys::kWebcamAnchor,
			 static_cast<int>(core::WebcamAnchor::MiddleCenter));
	obs_data_set_double(settings, core::keys::kWebcamOffsetX, 0.0);
	obs_data_set_double(settings, core::keys::kWebcamOffsetY, 0.0);

	obs_source_t *filter =
		obs_source_create(kFilterId, kFilterName, settings, nullptr);
	obs_data_release(settings);
	if (!filter) {
		blog(LOG_WARNING, "[TopScreen] could not create the webcam bubble filter");
		return false;
	}

	obs_source_filter_add(source, filter);
	obs_source_release(filter);
	return true;
}

void fit_bubble_item(obs_sceneitem_t *item, uint32_t base_width, uint32_t base_height)
{
	if (!item || !base_width || !base_height)
		return;

	const core::WebcamBubbleLayout layout = core::webcam_bubble_item_layout(
		core::WebcamConfig{}, static_cast<double>(base_width),
		static_cast<double>(base_height));

	struct vec2 bounds;
	vec2_set(&bounds, static_cast<float>(layout.diameter),
		 static_cast<float>(layout.diameter));
	obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
	obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
	obs_sceneitem_set_bounds(item, &bounds);

	obs_sceneitem_set_alignment(item, OBS_ALIGN_CENTER);
	struct vec2 pos;
	vec2_set(&pos, static_cast<float>(layout.center.x),
		 static_cast<float>(layout.center.y));
	obs_sceneitem_set_pos(item, &pos);
}

} // namespace

bool add_bubble_source()
{
	const char *camera_id = camera_input_id();
	if (!camera_id)
		return false;

	obs_source_t *scene_source = obs_frontend_get_current_scene();
	if (!scene_source)
		return false;
	obs_scene_t *scene = obs_scene_from_source(scene_source);
	if (!scene) {
		obs_source_release(scene_source);
		return false;
	}

	/* Idempotent: a second click must not create a second camera source or a
	 * second filter. It also repairs an existing source that lost its filter. */
	obs_sceneitem_t *existing = obs_scene_find_source(scene, kSourceName);
	if (existing) {
		obs_source_t *source = obs_sceneitem_get_source(existing);
		const bool ok = source && ensure_bubble_filter(source);
		obs_source_release(scene_source);
		return ok;
	}

	const std::string name = unique_source_name(scene);
	obs_source_t *camera = obs_source_create(camera_id, name.c_str(), nullptr, nullptr);
	if (!camera) {
		obs_source_release(scene_source);
		blog(LOG_WARNING, "[TopScreen] could not create the webcam source");
		return false;
	}

	const bool filter_ok = ensure_bubble_filter(camera);

	obs_sceneitem_t *item = obs_scene_add(scene, camera);
	obs_video_info ovi = {};
	if (item && obs_get_video_info(&ovi))
		fit_bubble_item(item, ovi.base_width, ovi.base_height);

	obs_source_release(camera);
	obs_source_release(scene_source);
	if (!filter_ok || !item) {
		blog(LOG_WARNING, "[TopScreen] webcam source was created without the bubble filter");
		return false;
	}

	blog(LOG_INFO, "[TopScreen] webcam bubble source added to the current scene");
	return true;
}

} // namespace topscreen::webcam_source
