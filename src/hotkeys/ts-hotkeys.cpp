#include "ts-hotkeys.hpp"

#include "core/profiles.hpp"
#include "ts-registry.hpp"
#include "ts-scene-profiles.hpp"

#include <obs-frontend-api.h>

#include <util/config-file.h>

namespace topscreen {

namespace {

/* Profile hotkeys are the only ones with a built-in default:
 * Ctrl+Shift+Alt+A .. Ctrl+Shift+Alt+J. */
const obs_key_t kProfileDefaults[10] = {
	OBS_KEY_A, OBS_KEY_B, OBS_KEY_C, OBS_KEY_D, OBS_KEY_E,
	OBS_KEY_F, OBS_KEY_G, OBS_KEY_H, OBS_KEY_I, OBS_KEY_J,
};

bool is_profile_action(int action)
{
	return action >= HK_ProfileA && action <= HK_ProfileJ;
}

/* Localized description of each hotkey. The label key is derived from the
 * stable ID ("TopScreen_ProfileA" -> "Hotkey.ProfileA"), so adding an action
 * only needs the enum + name table in ts-hotkey-actions.hpp. */
const char *hotkey_description(int action)
{
	if (action < 0 || action >= HK_Count)
		return "";

	return hotkey_label_or_id(obs_module_text(kHotkeyLabelKeys[action]), kHotkeyNames[action]);
}

using HotkeyInvoke = void (*)(HotkeyContext *);

void invoke_toggle_effects(HotkeyContext *t) { t->toggle_effects(); }
void invoke_toggle_background(HotkeyContext *t) { t->toggle_background(); }
void invoke_zoom_in(HotkeyContext *t) { t->zoom_in(); }
void invoke_zoom_out(HotkeyContext *t) { t->zoom_out(); }
void invoke_reset_zoom(HotkeyContext *t) { t->reset_zoom(); }
void invoke_next_preset(HotkeyContext *t) { t->next_preset(); }
void invoke_prev_preset(HotkeyContext *t) { t->prev_preset(); }
template <int Index> void invoke_profile(HotkeyContext *t) { t->select_profile(Index); }
void invoke_profile_save(HotkeyContext *t) { t->save_profile(); }
void invoke_toggle_highlight(HotkeyContext *t) { t->toggle_highlight(); }
void invoke_toggle_spotlight(HotkeyContext *t) { t->toggle_spotlight(); }
void invoke_toggle_magnifier(HotkeyContext *t) { t->toggle_magnifier(); }
void invoke_toggle_cursor(HotkeyContext *t) { t->toggle_cursor(); }
void invoke_next_profile(HotkeyContext *t) { t->next_profile(); }
void invoke_prev_profile(HotkeyContext *t) { t->prev_profile(); }
void invoke_toggle_click_sound(HotkeyContext *t) { t->toggle_click_sound(); }
void invoke_toggle_keys(HotkeyContext *t) { t->toggle_keys(); }

const HotkeyInvoke kHotkeyInvoke[HK_Count] = {
	invoke_toggle_effects,
	invoke_toggle_background,
	invoke_zoom_in,
	invoke_zoom_out,
	invoke_reset_zoom,
	invoke_next_preset,
	invoke_prev_preset,
	invoke_profile<0>,
	invoke_profile<1>,
	invoke_profile<2>,
	invoke_profile<3>,
	invoke_profile<4>,
	invoke_profile<5>,
	invoke_profile<6>,
	invoke_profile<7>,
	invoke_profile<8>,
	invoke_profile<9>,
	invoke_profile_save,
	invoke_toggle_highlight,
	invoke_toggle_spotlight,
	invoke_toggle_magnifier,
	invoke_toggle_cursor,
	invoke_next_profile,
	invoke_prev_profile,
	invoke_toggle_click_sound,
	invoke_toggle_keys,
};
} // namespace

namespace {
void frontend_profile_event(enum obs_frontend_event event, void *data);

void apply_scene_profile(obs_source_t *scene)
{
	if (!scene)
		return;
	const int profile = scene_profiles::get(scene);
	if (profile < 0)
		return;
	for (const auto &entry : registry_list()) {
		if (entry.control)
			entry.control->select_profile(profile);
	}
	blog(LOG_INFO, "[TopScreen] scene profile: %s -> %s",
	     obs_source_get_name(scene) ? obs_source_get_name(scene) : "?",
	     core::profile_label(profile));
}
}

std::atomic<int> Hotkeys::refs_{0};
HotkeyBinding Hotkeys::bindings_[HK_Count];
obs_hotkey_id Hotkeys::ids_[HK_Count] = {0};

void Hotkeys::register_all(HotkeyContext *ctx)
{
	hotkey_target_add(ctx);
	if (refs_.fetch_add(1) != 0)
		return; /* already registered process-wide */

	int registered = 0;
	for (int i = 0; i < HK_Count; ++i) {
		bindings_[i].action = i;
		ids_[i] = obs_hotkey_register_frontend(kHotkeyNames[i], hotkey_description(i),
						       ts_hotkey_callback, &bindings_[i]);
		if (ids_[i] && ids_[i] != OBS_INVALID_HOTKEY_ID)
			++registered;
		else
			ids_[i] = 0;
	}

	/* OBS saves plugin frontend hotkeys in the profile config but does not
	 * restore them for us. Load the user's bindings, then fill only the
	 * missing profile hotkeys with the Ctrl+Shift+Alt+A..J defaults. */
	load_saved_or_defaults();
	obs_frontend_add_event_callback(frontend_profile_event, nullptr);

	/* Frontend hotkeys show up flat in Settings > Hotkeys (not grouped under
	 * the filter instance: per-source registration is impossible for
	 * filters, which OBS always marks private). */
	blog(LOG_INFO, "[TopScreen] hotkeys registered: %d/%d (global, act on the active filter)",
	     registered, HK_Count);
	for (int i = 0; i < HK_Count; ++i)
		blog(LOG_INFO, "[TopScreen]   hotkey id=%d %s (%s)", static_cast<int>(ids_[i]),
		     kHotkeyNames[i], hotkey_description(i));
}

bool Hotkeys::apply_default_binding(int action)
{
	if (!is_profile_action(action) || !ids_[action])
		return false;

	obs_key_combination_t combo = {};
	combo.modifiers = INTERACT_CONTROL_KEY | INTERACT_SHIFT_KEY | INTERACT_ALT_KEY;
	combo.key = kProfileDefaults[action - HK_ProfileA];
	obs_hotkey_load_bindings(ids_[action], &combo, 1);
	return true;
}

void Hotkeys::load_saved_or_defaults()
{
	config_t *profile_config = obs_frontend_get_profile_config();
	int defaults = 0;

	for (int i = 0; i < HK_Count; ++i) {
		if (!ids_[i])
			continue;

		bool saved = false;
		if (profile_config) {
			const char *json =
				config_get_string(profile_config, "Hotkeys", kHotkeyNames[i]);
			if (json && *json) {
				obs_data_t *data = obs_data_create_from_json(json);
				if (data) {
					obs_data_array_t *bindings =
						obs_data_get_array(data, "bindings");
					if (bindings) {
						/* A saved empty array is intentional:
						 * the user cleared this hotkey. */
						obs_hotkey_load(ids_[i], bindings);
						obs_data_array_release(bindings);
						saved = true;
					}
					obs_data_release(data);
				} else {
					blog(LOG_WARNING,
					     "[TopScreen] invalid saved hotkey JSON for %s",
					     kHotkeyNames[i]);
				}
			}
		}

		if (!saved) {
			/* No saved entry in this OBS profile: clear the old binding
			 * first (important after a profile switch), then apply the
			 * profile default when applicable. */
			obs_hotkey_load_bindings(ids_[i], nullptr, 0);
			if (is_profile_action(i) && apply_default_binding(i))
				++defaults;
		}
	}

	if (defaults)
		blog(LOG_INFO,
		     "[TopScreen] applied default profile hotkeys Ctrl+Shift+Alt+A..J (%d)",
		     defaults);
}

void Hotkeys::reload_saved_bindings()
{
	load_saved_or_defaults();
}

void Hotkeys::save_bindings_to_profile()
{
	config_t *profile_config = obs_frontend_get_profile_config();
	if (!profile_config)
		return;

	int saved = 0;
	for (int i = 0; i < HK_Count; ++i) {
		if (!ids_[i])
			continue;
		obs_data_array_t *array = obs_hotkey_save(ids_[i]);
		if (!array)
			continue;
		obs_data_t *data = obs_data_create();
		obs_data_set_array(data, "bindings", array);
		const char *json = obs_data_get_json(data);
		if (json) {
			config_set_string(profile_config, "Hotkeys", kHotkeyNames[i], json);
			++saved;
		}
		obs_data_release(data);
		obs_data_array_release(array);
	}
	if (saved) {
		config_save_safe(profile_config, "tmp", nullptr);
		blog(LOG_INFO, "[TopScreen] hotkey bindings saved to the OBS profile (%d)",
		     saved);
	}
}

void Hotkeys::apply_default_profile_bindings()
{
	int applied = 0;
	for (int i = HK_ProfileA; i <= HK_ProfileJ; ++i) {
		if (apply_default_binding(i))
			++applied;
	}
	/* Persist the forced defaults; otherwise OBS would reload the old saved
	 * bindings after a restart and undo the button. */
	save_bindings_to_profile();
	blog(LOG_INFO, "[TopScreen] profile hotkeys restored to defaults (%d/10)", applied);
}

namespace {
void frontend_profile_event(enum obs_frontend_event event, void *data)
{
	UNUSED_PARAMETER(data);
	if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGED) {
		Hotkeys::reload_saved_bindings();
		blog(LOG_INFO, "[TopScreen] OBS profile changed: hotkey bindings reloaded");
	} else if (event == OBS_FRONTEND_EVENT_SCENE_CHANGED) {
		for (const auto &entry : registry_list()) {
			if (entry.control)
				entry.control->cancel_highlight();
		}
		obs_source_t *scene = obs_frontend_get_current_scene();
		if (!scene)
			return;
		apply_scene_profile(scene);
		obs_source_release(scene);
	} else if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		Hotkeys::apply_scene_profile_to_current();
	} else if (event == OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED ||
		   event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED) {
		obs_frontend_source_list scenes = {};
		obs_frontend_get_scenes(&scenes);
		std::vector<std::string> names;
		std::vector<std::string> uuids;
		names.reserve(scenes.sources.num);
		uuids.reserve(scenes.sources.num);
		for (size_t i = 0; i < scenes.sources.num; ++i) {
			obs_source_t *scene = scenes.sources.array[i];
			const char *name = obs_source_get_name(scene);
			if (name)
				names.emplace_back(name);
			const char *uuid = obs_source_get_uuid(scene);
			if (uuid && *uuid)
				uuids.emplace_back(uuid);
		}
		obs_frontend_source_list_free(&scenes);
		scene_profiles::prune(names);
		scene_profiles::prune_uuids(uuids);
	}
}
} // namespace

void Hotkeys::apply_scene_profile_to_current()
{
	obs_source_t *scene = obs_frontend_get_current_scene();
	if (!scene)
		return;
	apply_scene_profile(scene);
	obs_source_release(scene);
}

void Hotkeys::unregister_all(HotkeyContext *ctx)
{
	hotkey_target_remove(ctx);
	if (refs_.fetch_sub(1) != 1)
		return; /* other filters still use them */
	obs_frontend_remove_event_callback(frontend_profile_event, nullptr);
	for (int i = 0; i < HK_Count; ++i) {
		if (ids_[i])
			obs_hotkey_unregister(ids_[i]);
		ids_[i] = 0;
	}
}

void ts_hotkey_callback(void *data, obs_hotkey_id id, obs_hotkey_t *key, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(key);

	if (!pressed)
		return;

	auto *binding = static_cast<HotkeyBinding *>(data);
	if (!binding)
		return;

	/* Route to the filter selected in the dock (or the first active one). */
	HotkeyContext *target = hotkey_target_first();
	if (!target)
		return;

	blog(LOG_INFO, "[TopScreen] hotkey fired: %s",
	     binding->action < HK_Count ? kHotkeyNames[binding->action] : "?");

	const int action = binding->action;
	if (action < 0 || action >= HK_Count)
		return;
	kHotkeyInvoke[action](target);
}

} // namespace topscreen
