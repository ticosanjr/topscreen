#pragma once

#include <string>
#include <vector>

#include "core/config.hpp"
#include "core/preset.hpp"

namespace topscreen {

/**
 * Carrega e aplica presets embutidos (arquivos .json em data/presets).
 *
 * Os arquivos sao resolvidos via obs_module_file(), portanto funcionam tanto
 * instalados quanto em modo portable.
 */
class PresetStore {
public:
	/** Carrega os presets conhecidos. Retorna quantos foram carregados. */
	int load();

	int count() const { return static_cast<int>(presets_.size()); }
	int current() const { return current_; }
	const std::string &name(int index) const;

	/** Aplica o preset `index` sobre a config. Retorna false se invalido. */
	bool apply(int index, core::Config *cfg);

	/** Avanca/retrocede ciclicamente. Retorna o novo indice (-1 se vazio). */
	int next();
	int prev();

private:
	std::vector<core::Preset> presets_;
	int current_ = -1;
};

} // namespace topscreen
