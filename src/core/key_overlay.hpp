#pragma once

#include <string>
#include <vector>

namespace topscreen::core {

struct KeyOverlayConfig {
	bool enabled = true;
	bool modifiers_only = false;
	double ttl_ms = 2000.0;
	int max_keys = 6;
	int position = 0; // 0 = bottom_center, 1 = bottom_left, 2 = bottom_right, 3 = top_center
};

struct KeyRender {
	std::string label;
	double progress = 0.0; // 0..1
	bool modifier = false;
};

/** Formata um combo: {"Ctrl","Shift"} + "K" => "Ctrl + Shift + K". */
std::string format_combo(const std::vector<std::string> &modifiers, const std::string &key);

/**
 * Overlay de teclas.
 *
 * Mantem uma fila de teclas recentes (com TTL), permitindo mostrar o que esta
 * sendo digitado. Nao captura teclas (isso e especifico de plataforma); apenas
 * modela o estado exibido.
 */
class KeyOverlayModel {
public:
	void configure(const KeyOverlayConfig &cfg);

	/** Registra uma tecla. Repetir a mesma tecla reinicia o tempo. */
	void press(const std::string &label, bool is_modifier, double now_ms);

	void clear();

	/** Remove teclas expiradas. */
	void tick(double now_ms);

	int active_count() const { return static_cast<int>(entries_.size()); }

	/** Preenche ate `max` entradas (da mais antiga para a mais nova). */
	int collect(double now_ms, KeyRender *out, int max) const;

private:
	struct Entry {
		std::string label;
		double born_ms = 0.0;
		bool modifier = false;
	};

	std::vector<Entry> entries_;
	KeyOverlayConfig cfg_;
};

} // namespace topscreen::core
