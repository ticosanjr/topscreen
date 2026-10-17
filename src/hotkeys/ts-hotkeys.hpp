#pragma once

#include <obs-module.h>

namespace topscreen {

/** Acoes de atalho (indice estavel - nao reordenar sem migrar configs). */
enum HotkeyAction {
	HK_ToggleEffects = 0,
	HK_ToggleBackground,
	HK_ZoomIn,
	HK_ZoomOut,
	HK_ResetZoom,
	HK_NextPreset,
	HK_PrevPreset,
	HK_ProfileA,
	HK_ProfileB,
	HK_ProfileC,
	HK_ProfileD,
	HK_ProfileSave,
	HK_Count,
};

/** Interface implementada pelo filtro para reagir a atalhos. */
struct HotkeyContext {
	virtual ~HotkeyContext() = default;
	virtual void toggle_effects() = 0;
	virtual void toggle_background() = 0;
	virtual void zoom_in() = 0;
	virtual void zoom_out() = 0;
	virtual void reset_zoom() = 0;
	virtual void next_preset() = 0;
	virtual void prev_preset() = 0;
	/** Ativa um perfil (0..3) e enfileira o salvamento no perfil atual. */
	virtual void select_profile(int index) = 0;
	virtual void save_profile() = 0;
};

struct HotkeyBinding {
	HotkeyContext *ctx = nullptr;
	int action = 0;
};

void ts_hotkey_callback(void *data, obs_hotkey_id id, obs_hotkey_t *key, bool pressed);

/** Registra/desregistra os atalhos do TopScreen na fonte. */
class Hotkeys {
public:
	void register_all(obs_source_t *source, HotkeyContext *ctx);
	void unregister_all();

private:
	obs_hotkey_id ids_[HK_Count] = {};
	HotkeyBinding bindings_[HK_Count] = {};
};

} // namespace topscreen
