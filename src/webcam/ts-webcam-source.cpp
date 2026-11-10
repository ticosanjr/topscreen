#include "ts-webcam-source.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <cstring>

namespace topscreen::webcam_source {

namespace {

constexpr const char *kSourceName = "TopScreen Webcam";
constexpr const char *kFilterId = "topscreen_webcam_bubble";

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
	if (obs_scene_find_source(scene, kSourceName)) {
		obs_source_release(scene_source);
		return true;
	}

	obs_source_t *camera = obs_source_create(camera_id, kSourceName, nullptr, nullptr);
	if (!camera) {
		obs_source_release(scene_source);
		blog(LOG_WARNING, "[TopScreen] could not create the webcam source");
		return false;
	}

	obs_source_t *filter =
		obs_source_create(kFilterId, "Webcam Bubble", nullptr, nullptr);
	if (filter) {
		obs_source_filter_add(camera, filter);
		obs_source_release(filter);
	}

	obs_sceneitem_t *item = obs_scene_add(scene, camera);
	obs_video_info ovi = {};
	if (item && obs_get_video_info(&ovi) && ovi.base_width && ovi.base_height) {
		struct vec2 bounds;
		vec2_set(&bounds, static_cast<float>(ovi.base_width),
			 static_cast<float>(ovi.base_height));
		obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
		obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
		obs_sceneitem_set_bounds(item, &bounds);
		struct vec2 pos;
		vec2_set(&pos, 0.0f, 0.0f);
		obs_sceneitem_set_pos(item, &pos);
	}

	obs_source_release(camera);
	obs_source_release(scene_source);
	blog(LOG_INFO, "[TopScreen] webcam bubble source added to the current scene");
	return item != nullptr;
}

} // namespace topscreen::webcam_source
