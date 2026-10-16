#include "ts-hotkeys.hpp"

namespace topscreen {

namespace {

const char *const kHotkeyNames[HK_Count] = {
	"TopScreen.ToggleEffects",  "TopScreen.ToggleBackground", "TopScreen.ZoomIn",
	"TopScreen.ZoomOut",        "TopScreen.ResetZoom",        "TopScreen.NextPreset",
	"TopScreen.PrevPreset",
};

const char *const kHotkeyDescriptions[HK_Count] = {
	"Hotkey.ToggleEffects", "Hotkey.ToggleBackground", "Hotkey.ZoomIn",
	"Hotkey.ZoomOut",       "Hotkey.ResetZoom",        "Hotkey.NextPreset",
	"Hotkey.PrevPreset",
};

} // namespace

void Hotkeys::register_all(obs_source_t *source, HotkeyContext *ctx)
{
	for (int i = 0; i < HK_Count; ++i) {
		bindings_[i].ctx = ctx;
		bindings_[i].action = i;
		ids_[i] = obs_hotkey_register_source(source, kHotkeyNames[i],
						     obs_module_text(kHotkeyDescriptions[i]),
						     ts_hotkey_callback, &bindings_[i]);
	}
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
	default:
		break;
	}
}

} // namespace topscreen
