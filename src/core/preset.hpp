#pragma once

#include "config.hpp"
#include "json.hpp"

#include <string>

namespace topscreen::core {

/**
 * Preset: envelope JSON com metadados e um bloco `settings` contendo as chaves
 * de configuracao (mesmas chaves do obs_data - ver core/keys.hpp).
 */
struct Preset {
	std::string name;
	std::string description;

	/** Aplica os valores do preset sobre a config (valores ausentes sao mantidos). */
	bool apply(Config *cfg, std::string *error) const;

	/** Faz o parse do texto JSON de um preset. */
	static bool from_json(const std::string &text, Preset *out, std::string *error);

	/** Objeto `settings` do preset (para inspecao/testes). */
	const json::Value &settings() const { return settings_; }

private:
	json::Value settings_;
};

} // namespace topscreen::core
