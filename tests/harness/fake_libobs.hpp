#pragma once

// "libobs fake" para testes de integracao (sem OBS real).
//
// O codigo do plugin e compilado contra os headers REAIS do OBS e linkado
// contra estas implementacoes. Os tipos publicos sao opacos, entao podemos
// definir os structs aqui.

#include <obs-module.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct obs_data {
	std::map<std::string, bool> bools;
	std::map<std::string, long long> ints;
	std::map<std::string, double> doubles;
	std::map<std::string, std::string> strings;
	std::map<std::string, obs_data_t *> objs;
};

struct obs_property {
	std::string name;
};

struct obs_properties {
	std::map<std::string, obs_property *> items;
};

struct obs_source {
	const obs_source_info *info = nullptr;
	void *data = nullptr;
	obs_data_t *settings = nullptr;
	obs_source_t *parent = nullptr;
	uint32_t base_w = 0;
	uint32_t base_h = 0;
};

namespace fakeobs {

/** Define o diretorio de dados resolvido por obs_module_file(). */
void set_data_dir(const std::string &dir);

/** Cria uma fonte fake (sem registrar). */
obs_source_t *make_source(const obs_source_info *info, obs_source_t *parent, uint32_t w, uint32_t h);

/** Ultima fonte registrada por obs_register_source(). */
const obs_source_info *registered_source();

/** Indice do atalho registrado com este nome (-1 se nao houver). */
int find_hotkey(const char *name);

/** Dispara o atalho no indice (pressed=true/false). */
void invoke_hotkey(int index, bool pressed);

int hotkey_count();

} // namespace fakeobs
