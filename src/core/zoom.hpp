#pragma once

#include "easing.hpp"
#include "spring.hpp"
#include "types.hpp"

namespace topscreen::core {

struct ZoomConfig {
	int mode = static_cast<int>(ZoomMode::Auto);
	double amount = 1.6;         // zoom alvo no modo auto
	double duration_ms = 450.0;  // velocidade da animacao
	double idle_timeout_ms = 1500.0; // sem foco -> volta ao normal
	double pan_amount = 0.25;    // sensibilidade do pan
	int easing = EASE_IN_OUT_CUBIC;
};

struct ZoomRender {
	double zoom = 1.0;
	double focus_x = 0.5;
	double focus_y = 0.5;
};

/**
 * Modelo de zoom/pan.
 *
 * Combina zoom manual (passo a passo por atalho) com zoom automatico
 * (foco por clique/seguir cursor). Usa molas para tudo, evitando saltos.
 * Todo foco e limitado por clamp_focus() para nunca amostrar fora da tela.
 */
class ZoomModel {
public:
	void configure(const ZoomConfig &cfg);
	void reset(double now_ms);

	/** Foco por clique (auto-zoom). */
	void focus_on(double x, double y, double now_ms);

	/** Foco seguindo o cursor. */
	void set_cursor(double x, double y, double now_ms);

	/** Passo manual: factor > 1 aproxima, factor < 1 afasta. */
	void manual_step(double factor);
	void set_manual(double value);
	double manual() const { return manual_; }

	void update(double now_ms);

	ZoomRender render() const;

	/** Velocidade atual do zoom (para motion blur). */
	double zoom_velocity() const { return zoom_.velocity(); }

private:
	double stiffness() const;

	ZoomConfig cfg_;
	Spring zoom_;
	Spring fx_;
	Spring fy_;
	double manual_ = 1.0;
	double last_focus_ms_ = 0.0;
	double last_update_ms_ = 0.0;
	bool started_ = false;
};

} // namespace topscreen::core
