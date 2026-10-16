#pragma once

#include "spring.hpp"
#include "types.hpp"

namespace topscreen::core {

struct CursorConfig {
	bool enabled = true;
	int style = static_cast<int>(CursorStyle::MacOs);
	double size = 1.4;     // multiplicador de tamanho
	bool shadow = true;
	double smooth = 0.0;   // 0 = segue o mouse exatamente
	double sway = 0.0;     // 0..1 -> inclinacao na curva
	bool hide_idle = true; // oculta quando parado
	double idle_delay_ms = 1200.0;
	bool loop = false; // volta ao inicio quando parado
	double loop_ms = 4000.0;
};

struct CursorRender {
	double x = 0.5;
	double y = 0.5;
	double opacity = 1.0;
	double rotation = 0.0; // rad
	double size = 1.0;
	bool visible = true;
};

/**
 * Modelo de cursor: suavizacao, sway, idle e loop.
 *
 * Trabalha em coordenadas de conteudo (0..1). Nao desenha nada; apenas produz
 * o estado que o compositor usa para renderizar.
 */
class CursorModel {
public:
	void configure(const CursorConfig &cfg);

	/** Define a posicao inicial (e o ponto de retorno do loop). */
	void reset(double x, double y, double now_ms);

	/** Define a posicao alvo (posicao real do ponteiro). */
	void set_target(double x, double y, double now_ms);

	/** Avanca a simulacao ate now_ms. */
	void update(double now_ms);

	CursorRender render() const;
	bool visible() const { return render().visible; }

private:
	double stiffness() const;

	CursorConfig cfg_;
	Spring sx_;
	Spring sy_;
	double last_move_ms_ = 0.0;
	double last_update_ms_ = 0.0;
	double start_x_ = 0.5;
	double start_y_ = 0.5;
	double rotation_ = 0.0;
	bool started_ = false;
};

} // namespace topscreen::core
