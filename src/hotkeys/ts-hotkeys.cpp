#include "ts-hotkeys.hpp"

#include "ts-registry.hpp"

namespace topscreen {

namespace {

/* Localized description of each hotkey (literals for the translation test).
 * Every label starts with the literal ID ("TopScreen_ProfileA") so the OBS
 * hotkey list is searchable by the same prefix as the registered ID. */
const char *hotkey_description(int action)
{
	switch (action) {
	case HK_ToggleEffects:
		return obs_module_text("Hotkey.ToggleEffects");
	case HK_ToggleBackground:
		return obs_module_text("Hotkey.ToggleBackground");
	case HK_ZoomIn:
		return obs_module_text("Hotkey.ZoomIn");
	case HK_ZoomOut:
		return obs_module_text("Hotkey.ZoomOut");
	case HK_ResetZoom:
		return obs_module_text("Hotkey.ResetZoom");
	case HK_NextPreset:
		return obs_module_text("Hotkey.NextPreset");
	case HK_PrevPreset:
		return obs_module_text("Hotkey.PrevPreset");
	case HK_ProfileA:
		return obs_module_text("Hotkey.ProfileA");
	case HK_ProfileB:
		return obs_module_text("Hotkey.ProfileB");
	case HK_ProfileC:
		return obs_module_text("Hotkey.ProfileC");
	case HK_ProfileD:
		return obs_module_text("Hotkey.ProfileD");
	case HK_ProfileE:
		return obs_module_text("Hotkey.ProfileE");
	case HK_ProfileF:
		return obs_module_text("Hotkey.ProfileF");
	case HK_ProfileG:
		return obs_module_text("Hotkey.ProfileG");
	case HK_ProfileH:
		return obs_module_text("Hotkey.ProfileH");
	case HK_ProfileI:
		return obs_module_text("Hotkey.ProfileI");
	case HK_ProfileJ:
		return obs_module_text("Hotkey.ProfileJ");
	case HK_ProfileSave:
		return obs_module_text("Hotkey.ProfileSave");
	default:
		return "";
	}
}

} // namespace

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

	/* Frontend hotkeys show up flat in Settings > Hotkeys (not grouped under
	 * the filter instance: per-source registration is impossible for
	 * filters, which OBS always marks private). */
	blog(LOG_INFO, "[TopScreen] hotkeys registered: %d/%d (global, act on the active filter)",
	     registered, HK_Count);
	for (int i = 0; i < HK_Count; ++i)
		blog(LOG_INFO, "[TopScreen]   hotkey id=%d %s (%s)", static_cast<int>(ids_[i]),
		     kHotkeyNames[i], hotkey_description(i));
}

void Hotkeys::unregister_all(HotkeyContext *ctx)
{
	hotkey_target_remove(ctx);
	if (refs_.fetch_sub(1) != 1)
		return; /* other filters still use them */
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
	default:
		break;
	}
}

} // namespace topscreen
