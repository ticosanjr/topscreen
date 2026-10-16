#pragma once

#include "easing.hpp"
#include "types.hpp"

#include <cstdint>

namespace topscreen::core {

struct ClickConfig {
	int effect = static_cast<int>(ClickEffect::Ripple);
	double size = 1.0;
	double duration_ms = 650.0;
	uint32_t color = 0xFFFFFFFF; // 0xAABBGGRR
	int easing = EASE_OUT_CUBIC;
};

struct ClickRender {
	double x = 0.0;
	double y = 0.0;
	double progress = 0.0; // 0..1
	double eased = 0.0;    // easing aplicado
	double size = 1.0;
	uint32_t color = 0xFFFFFFFF;
	int effect = 0;
};

/**
 * Modelo de efeitos de clique.
 *
 * Mantem uma fila de animacoes ativas (esquerdo/direito independentes) e
 * descarta as expiradas. Nao desenha; o compositor consome o resultado.
 */
class ClickFxModel {
public:
	static constexpr int kMaxSlots = 8;

	void set_left(const ClickConfig &cfg) { left_ = cfg; }
	void set_right(const ClickConfig &cfg) { right_ = cfg; }

	void spawn(double x, double y, double now_ms, bool right);

	/** Remove animacoes expiradas. */
	void tick(double now_ms);

	int active_count() const;

	/** Preenche ate `max` entradas e retorna quantas foram escritas. */
	int collect(double now_ms, ClickRender *out, int max) const;

private:
	struct Slot {
		bool used = false;
		double x = 0.0;
		double y = 0.0;
		double born_ms = 0.0;
		ClickConfig cfg;
	};

	Slot slots_[kMaxSlots];
	int next_ = 0;
	ClickConfig left_;
	ClickConfig right_;
};

} // namespace topscreen::core
