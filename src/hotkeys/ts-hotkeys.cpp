#include "ts-hotkeys.hpp"

#include "ts-registry.hpp"

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

/* Localized description of each hotkey (literals for the translation test).
 * Every label starts with the literal ID ("TopScreen_ProfileA") so the OBS
 * hotkey list is searchable by the same prefix as the registered ID. */
const char *hotkey_description(int action)
{
	switch (action) {
	case HK_ToggleEffects:
		return hotkey_label_or_id(obs_module_text("Hotkey.ToggleEffects"), kHotkeyNames[action]);
	case HK_ToggleBackground:
		return hotkey_label_or_id(obs_module_text("Hotkey.ToggleBackground"), kHotkeyNames[action]);
	case HK_ZoomIn:
		return hotkey_label_or_id(obs_module_text("Hotkey.ZoomIn"), kHotkeyNames[action]);
	case HK_ZoomOut:
		return hotkey_label_or_id(obs_module_text("Hotkey.ZoomOut"), kHotkeyNames[action]);
	case HK_ResetZoom:
		return hotkey_label_or_id(obs_module_text("Hotkey.ResetZoom"), kHotkeyNames[action]);
	case HK_NextPreset:
		return hotkey_label_or_id(obs_module_text("Hotkey.NextPreset"), kHotkeyNames[action]);
	case HK_PrevPreset:
		return hotkey_label_or_id(obs_module_text("Hotkey.PrevPreset"), kHotkeyNames[action]);
	case HK_ProfileA:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileA"), kHotkeyNames[action]);
	case HK_ProfileB:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileB"), kHotkeyNames[action]);
	case HK_ProfileC:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileC"), kHotkeyNames[action]);
	case HK_ProfileD:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileD"), kHotkeyNames[action]);
	case HK_ProfileE:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileE"), kHotkeyNames[action]);
	case HK_ProfileF:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileF"), kHotkeyNames[action]);
	case HK_ProfileG:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileG"), kHotkeyNames[action]);
	case HK_ProfileH:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileH"), kHotkeyNames[action]);
	case HK_ProfileI:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileI"), kHotkeyNames[action]);
	case HK_ProfileJ:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileJ"), kHotkeyNames[action]);
	case HK_ProfileSave:
		return hotkey_label_or_id(obs_module_text("Hotkey.ProfileSave"), kHotkeyNames[action]);
	case HK_ToggleHighlight:
		return hotkey_label_or_id(obs_module_text("Hotkey.ToggleHighlight"), kHotkeyNames[action]);
	case HK_ToggleSpotlight:
		return hotkey_label_or_id(obs_module_text("Hotkey.ToggleSpotlight"), kHotkeyNames[action]);
	case HK_ToggleMagnifier:
		return hotkey_label_or_id(obs_module_text("Hotkey.ToggleMagnifier"), kHotkeyNames[action]);
	case HK_ToggleCursor:
		return hotkey_label_or_id(obs_module_text("Hotkey.ToggleCursor"), kHotkeyNames[action]);
	default:
		return "";
	}
}

} // namespace

namespace {
void frontend_profile_event(enum obs_frontend_event event, void *data);
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
	}
}
} // namespace

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

	switch (binding->action) {
	case HK_ToggleEffects:
		target->toggle_effects();
		break;
	case HK_ToggleBackground:
		target->toggle_background();
		break;
	case HK_ZoomIn:
		target->zoom_in();
		break;
	case HK_ZoomOut:
		target->zoom_out();
		break;
	case HK_ResetZoom:
		target->reset_zoom();
		break;
	case HK_NextPreset:
		target->next_preset();
		break;
	case HK_PrevPreset:
		target->prev_preset();
		break;
	case HK_ProfileA:
		target->select_profile(0);
		break;
	case HK_ProfileB:
		target->select_profile(1);
		break;
	case HK_ProfileC:
		target->select_profile(2);
		break;
	case HK_ProfileD:
		target->select_profile(3);
		break;
	case HK_ProfileE:
		target->select_profile(4);
		break;
	case HK_ProfileF:
		target->select_profile(5);
		break;
	case HK_ProfileG:
		target->select_profile(6);
		break;
	case HK_ProfileH:
		target->select_profile(7);
		break;
	case HK_ProfileI:
		target->select_profile(8);
		break;
	case HK_ProfileJ:
		target->select_profile(9);
		break;
	case HK_ProfileSave:
		target->save_profile();
		break;
	case HK_ToggleHighlight:
		target->toggle_highlight();
		break;
	case HK_ToggleSpotlight:
		target->toggle_spotlight();
		break;
	case HK_ToggleMagnifier:
		target->toggle_magnifier();
		break;
	case HK_ToggleCursor:
		target->toggle_cursor();
		break;
	default:
		break;
	}
}

} // namespace topscreen
