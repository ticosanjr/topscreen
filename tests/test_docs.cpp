#include "test_framework.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_SOURCE_DIR
#define TOPSCREEN_SOURCE_DIR "."
#endif

namespace {

std::string read_text(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

/* Extrai os valores das constantes de src/core/keys.hpp. */
std::set<std::string> keys_from_header(const std::string &path)
{
	std::set<std::string> keys;
	std::ifstream in(path);
	std::string line;
	while (std::getline(in, line)) {
		const auto q1 = line.find('"');
		if (q1 == std::string::npos)
			continue;
		const auto q2 = line.find('"', q1 + 1);
		if (q2 == std::string::npos)
			continue;
		keys.insert(line.substr(q1 + 1, q2 - q1 - 1));
	}
	return keys;
}

bool contains(const std::string &haystack, const std::string &needle)
{
	return haystack.find(needle) != std::string::npos;
}

} // namespace

/* A referencia de configuracao deve documentar TODAS as chaves. */
TEST(docs_documentam_todas_as_chaves)
{
	const std::string keys_hpp = std::string(TOPSCREEN_SOURCE_DIR) + "/src/core/keys.hpp";
	const std::string config_md = std::string(TOPSCREEN_SOURCE_DIR) + "/docs/CONFIGURACAO.md";

	const auto keys = keys_from_header(keys_hpp);
	CHECK(!keys.empty());

	const std::string doc = read_text(config_md);
	CHECK(!doc.empty());

	for (const auto &k : keys) {
		++tst::checks();
		if (!contains(doc, k))
			tst::fail(__FILE__, __LINE__, "chave ausente em docs/CONFIGURACAO.md: " + k);
	}
}

/* Os atalhos do codigo devem estar na referencia com o id TopScreen.<Acao>. */
TEST(docs_referencia_cita_atalhos)
{
	const std::string hotkeys =
		read_text(std::string(TOPSCREEN_SOURCE_DIR) + "/src/hotkeys/ts-hotkeys.cpp");
	const std::string doc = read_text(std::string(TOPSCREEN_SOURCE_DIR) + "/docs/CONFIGURACAO.md");
	CHECK(!doc.empty());

	const char *actions[] = {"ToggleEffects", "ToggleBackground", "ZoomIn",
				 "ZoomOut",       "ResetZoom",        "NextPreset", "PrevPreset"};
	for (const char *a : actions) {
		++tst::checks();
		if (!contains(hotkeys, a))
			tst::fail(__FILE__, __LINE__, std::string("acao de atalho ausente no codigo: ") + a);

		++tst::checks();
		const std::string id = std::string("TopScreen.") + a;
		if (!contains(doc, id))
			tst::fail(__FILE__, __LINE__, "atalho ausente em docs/CONFIGURACAO.md: " + id);
	}
}
