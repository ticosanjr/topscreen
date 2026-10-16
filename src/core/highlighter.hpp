#pragma once

#include <cstdint>

namespace topscreen::core {

struct HighlighterConfig {
	bool enabled = true;
	double hold_ms = 900.0; // tempo visivel apos soltar
	double fade_ms = 300.0; // duracao do fade
	uint32_t color = 0x6600E0FF; // 0xAABBGGRR (amarelo translucido)
	double min_span = 0.02;      // arrasto minimo (evita marcar cliques)
};

struct HighlightRender {
	bool active = false;
	double x0 = 0.0, y0 = 0.0, x1 = 0.0, y1 = 0.0; // normalizado (x0<x1, y0<y1)
	double alpha = 0.0;
	uint32_t color = 0;
};

/**
 * Marca-texto de arrasto.
 *
 * Enquanto o botao esquerdo esta pressionado, registra a regiao arrastada;
 * depois de soltar, mantem visivel por `hold_ms` e desvanece em `fade_ms`.
 */
class HighlighterModel {
public:
	void configure(const HighlighterConfig &cfg);

	void begin(double x, double y, double now);
	void move(double x, double y, double now);
	void end(double now);
	void update(double now);

	HighlightRender render() const;

private:
	HighlighterConfig cfg_;
	bool dragging_ = false;
	bool has_ = false;
	double sx_ = 0.0, sy_ = 0.0;
	double ex_ = 0.0, ey_ = 0.0;
	double ended_ms_ = 0.0;
	double alpha_ = 0.0;
};

} // namespace topscreen::core
