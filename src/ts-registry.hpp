#pragma once

#include <string>

namespace topscreen {

/**
 * Interface de controle do filtro, exposta ao dock (thread da UI).
 *
 * Todas as implementacoes devem ser seguras para chamada de qualquer thread:
 * as mutacoes sao enfileiradas e aplicadas no video_tick (thread de video).
 */
struct FilterControl {
	virtual ~FilterControl() = default;

	virtual bool effects_enabled() const = 0;
	virtual void set_effects_enabled(bool on) = 0;

	virtual int preset_count() const = 0;
	virtual std::string preset_name(int index) const = 0;
	virtual int current_preset() const = 0;
	virtual void apply_preset(int index) = 0;

	virtual int input_tier() const = 0;

	/* Perfis (4 slots fixos). select_profile/save_profile tambem existem em
	 * HotkeyContext (mesma assinatura); a instancia implementa uma vez. */
	virtual int profile_count() const = 0;
	virtual int current_profile() const = 0;
	virtual void select_profile(int index) = 0;
	virtual void save_profile() = 0;
};

/** Registra/desregistra um filtro ativo. */
void registry_add(FilterControl *control);
void registry_remove(FilterControl *control);

/** Primeiro filtro ativo (ou nullptr). Usado pelo dock. */
FilterControl *registry_first();

} // namespace topscreen
