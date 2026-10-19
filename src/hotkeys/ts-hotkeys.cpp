#include "ts-hotkeys.hpp"

namespace topscreen {

namespace {

const char *const kHotkeyNames[HK_Count] = {
	"TopScreen.ToggleEffects",  "TopScreen.ToggleBackground", "TopScreen.ZoomIn",
	"TopScreen.ZoomOut",        "TopScreen.ResetZoom",        "TopScreen.NextPreset",
	"TopScreen.PrevPreset",     "TopScreen.ProfileA",         "TopScreen.ProfileB",
	"TopScreen.ProfileC",       "TopScreen.ProfileD",         "TopScreen.ProfileE",
	"TopScreen.ProfileF",       "TopScreen.ProfileG",         "TopScreen.ProfileH",
	"TopScreen.ProfileI",       "TopScreen.ProfileJ",         "TopScreen.ProfileSave",
};

/* Localized description of each hotkey (literals for the translation test). */
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

void Hotkeys::register_all(obs_source_t *source, HotkeyContext *ctx)
{
	int registered = 0;
	for (int i = 0; i < HK_Count; ++i) {
		bindings_[i].ctx = ctx;
		bindings_[i].action = i;
		ids_[i] = obs_hotkey_register_source(source, kHotkeyNames[i], hotkey_description(i),
						     ts_hotkey_callback, &bindings_[i]);
		if (ids_[i])
			++registered;
	}

	/* Logged so a missing "TopScreen" section in Settings > Hotkeys can be
	 * told apart from a registration failure: the OBS UI groups these under
	 * the filter instance name. */
	blog(LOG_INFO, "[TopScreen] hotkeys registered: %d/%d", registered, HK_Count);
	for (int i = 0; i < HK_Count; ++i)
		blog(LOG_INFO, "[TopScreen]   hotkey id=%d %s (%s)", static_cast<int>(ids_[i]),
		     kHotkeyNames[i], hotkey_description(i));
}

void Hotkeys::unregister_all()
{
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
	if (!binding || !binding->ctx)
		return;

	blog(LOG_INFO, "[TopScreen] hotkey fired: %s",
	     binding->action < HK_Count ? kHotkeyNames[binding->action] : "?");

	switch (binding->action) {
	case HK_ToggleEffects:
		binding->ctx->toggle_effects();
		break;
	case HK_ToggleBackground:
		binding->ctx->toggle_background();
		break;
	case HK_ZoomIn:
		binding->ctx->zoom_in();
		break;
	case HK_ZoomOut:
		binding->ctx->zoom_out();
		break;
	case HK_ResetZoom:
		binding->ctx->reset_zoom();
		break;
	case HK_NextPreset:
		binding->ctx->next_preset();
		break;
	case HK_PrevPreset:
		binding->ctx->prev_preset();
		break;
	case HK_ProfileA:
		binding->ctx->select_profile(0);
		break;
	case HK_ProfileB:
		binding->ctx->select_profile(1);
		break;
	case HK_ProfileC:
		binding->ctx->select_profile(2);
		break;
	case HK_ProfileD:
		binding->ctx->select_profile(3);
		break;
	case HK_ProfileE:
		binding->ctx->select_profile(4);
		break;
	case HK_ProfileF:
		binding->ctx->select_profile(5);
		break;
	case HK_ProfileG:
		binding->ctx->select_profile(6);
		break;
	case HK_ProfileH:
		binding->ctx->select_profile(7);
		break;
	case HK_ProfileI:
		binding->ctx->select_profile(8);
		break;
	case HK_ProfileJ:
		binding->ctx->select_profile(9);
		break;
	case HK_ProfileSave:
		binding->ctx->save_profile();
		break;
	default:
		break;
	}
}

} // namespace topscreen
