#include "test_framework.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

namespace {

/* Le os nomes de uniform declarados no .effect (ignora comentarios). */
std::set<std::string> read_uniforms(bool *has_draw_technique)
{
	std::set<std::string> names;
	*has_draw_technique = false;

	const std::string path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/effects/topscreen.effect";
	std::ifstream in(path);
	if (!in)
		return names;

	std::string line;
	while (std::getline(in, line)) {
		const auto comment = line.find("//");
		if (comment != std::string::npos)
			line = line.substr(0, comment);

		if (line.find("technique Draw") != std::string::npos)
			*has_draw_technique = true;

		std::istringstream ss(line);
		std::string kw;
		ss >> kw;
		if (kw != "uniform")
			continue;

		std::string type, name;
		ss >> type >> name;
		if (name.empty())
			continue;

		const auto semi = name.find(';');
		if (semi != std::string::npos)
			name = name.substr(0, semi);
		const auto bracket = name.find('[');
		if (bracket != std::string::npos)
			name = name.substr(0, bracket);

		if (!name.empty())
			names.insert(name);
	}

	return names;
}

} // namespace

/*
 * O compositor (ts-compositor.cpp) faz lookup destes uniforms por nome, sem
 * checagem em runtime. Se o shader renomear/remover um deles, o efeito falha
 * em silencio. Este teste pega essa divergencia.
 */
TEST(shader_tem_uniforms_esperados)
{
	bool has_draw = false;
	const std::set<std::string> uniforms = read_uniforms(&has_draw);

	CHECK(has_draw);
	CHECK(!uniforms.empty());

	const char *required[] = {
		"ViewProj",       "image",          "target_size",   "content_pad",
		"corner_radius",  "zoom",           "zoom_focus",    "bg_a",
		"bg_b",           "bg_mode",        "shadow_alpha",  "shadow_blur",
		"shadow_offset",  "cursor_enabled", "cursor_pos",    "cursor_scale",
		"cursor_opacity", "cursor_rotation", "cursor_shadow", "click_count",
		"click_data",     "click_color",    "click_meta",
	};

	for (const char *name : required) {
		++tst::checks();
		if (uniforms.find(name) == uniforms.end())
			tst::fail(__FILE__, __LINE__, std::string("uniform ausente no shader: ") + name);
	}
}

/* O shader deve conter ao menos uma tecnica util para o filtro. */
TEST(shader_tem_tecnica_draw)
{
	bool has_draw = false;
	read_uniforms(&has_draw);
	CHECK(has_draw);
}
