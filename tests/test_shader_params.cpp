#include "test_framework.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

namespace {

std::string effect_path()
{
	return std::string(TOPSCREEN_TEST_DATA_DIR) + "/effects/topscreen.effect";
}

/* Reads the .effect removing line comments. */
std::string read_effect()
{
	std::ifstream in(effect_path());
	std::ostringstream out;
	std::string line;
	while (std::getline(in, line)) {
		const auto comment = line.find("//");
		if (comment != std::string::npos)
			line = line.substr(0, comment);
		out << line << '\n';
	}
	return out.str();
}

/* Reads the uniform names declared in the .effect (ignores comments). */
std::set<std::string> read_uniforms(bool *has_draw_technique)
{
	std::set<std::string> names;
	*has_draw_technique = false;

	std::istringstream in(read_effect());
	std::string line;
	while (std::getline(in, line)) {
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
 * The compositor (ts-compositor.cpp) looks up these uniforms by name, with no
 * runtime check. If the shader renames/removes one of them, the effect fails
 * silently. This test catches that divergence.
 */
TEST(shader_has_expected_uniforms)
{
	bool has_draw = false;
	const std::set<std::string> uniforms = read_uniforms(&has_draw);

	CHECK(has_draw);
	CHECK(!uniforms.empty());

	const char *required[] = {
		"ViewProj",       "image",          "target_size",   "content_pad",
		"corner_radius",  "zoom",           "zoom_focus",    "bg_a",
		"bg_b",           "bg_mode",        "bg_tex",        "bg_tex_enabled",
		"bg_tex_aspect",  "bg_blur",       "bg_blur_type",  "bg_keep_aspect",        "shadow_alpha",  "shadow_blur",
		"shadow_offset",  "cursor_enabled", "cursor_pos",    "cursor_scale",
		"cursor_opacity", "cursor_rotation", "cursor_shadow", "click_count",
		"cursor_tex",     "cursor_tex_enabled", "cursor_cell_off", "cursor_cell_scale",
		"cursor_hotspot",
		"zoom_blur",      "spotlight_enabled", "spotlight_radius", "spotlight_dim",
		"highlight_active", "highlight_rect", "highlight_color", "highlight_alpha",
		"click0_pos",     "click0_prog",    "click0_size",   "click0_color",
		"click0_type",    "click1_pos",     "click1_prog",   "click1_size",
		"click1_color",   "click1_type",    "click2_pos",    "click2_prog",
		"click2_size",    "click2_color",   "click2_type",   "click3_pos",
		"click3_prog",    "click3_size",    "click3_color",  "click3_type",
	};

	for (const char *name : required) {
		++tst::checks();
		if (uniforms.find(name) == uniforms.end())
			tst::fail(__FILE__, __LINE__, std::string("uniform missing from shader: ") + name);
	}
}

/* The shader must contain the technique used by the filter. */
TEST(shader_has_draw_technique)
{
	bool has_draw = false;
	read_uniforms(&has_draw);
	CHECK(has_draw);
}

namespace {

std::string read_text_file(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream out;
	out << in.rdbuf();
	return out.str();
}

/* Basic structural checks of a .effect. */
void check_effect_structure(const std::string &name)
{
	const std::string text = read_text_file(std::string(TOPSCREEN_TEST_DATA_DIR) + "/effects/" + name);
	++tst::checks();
	if (text.empty()) {
		tst::fail(__FILE__, __LINE__, "effect empty or missing: " + name);
		return;
	}

	int balance = 0;
	for (char c : text) {
		if (c == '{')
			++balance;
		else if (c == '}')
			--balance;
	}
	++tst::checks();
	if (balance != 0)
		tst::fail(__FILE__, __LINE__, "unbalanced braces in " + name);

	++tst::checks();
	if (text.find("technique ") == std::string::npos)
		tst::fail(__FILE__, __LINE__, "no technique in " + name);
	++tst::checks();
	if (text.find("pass") == std::string::npos)
		tst::fail(__FILE__, __LINE__, "no pass in " + name);
	++tst::checks();
	if (text.find("vertex_shader") == std::string::npos)
		tst::fail(__FILE__, __LINE__, "no vertex_shader in " + name);
	++tst::checks();
	if (text.find("pixel_shader") == std::string::npos)
		tst::fail(__FILE__, __LINE__, "no pixel_shader in " + name);
	++tst::checks();
	if (text.find("uniform float4x4 ViewProj") == std::string::npos)
		tst::fail(__FILE__, __LINE__, "no ViewProj in " + name);
}

} // namespace

TEST(shader_effects_structure)
{
	check_effect_structure("topscreen.effect");
	check_effect_structure("text.effect");
}

/*
 * Compatibility invariants: OBS shaders do not use uniform arrays or loops,
 * and the translator may not cover sign()/length(). We keep the same style to
 * reduce the risk of silent runtime failure.
 */
TEST(shader_avoids_risky_constructs)
{
	const std::string text = read_effect();

	++tst::checks();
	if (text.find("uniform") != std::string::npos) {
		// no uniform declaration may contain '[' (array)
		std::istringstream in(text);
		std::string line;
		while (std::getline(in, line)) {
			std::istringstream ss(line);
			std::string kw;
			ss >> kw;
			if (kw == "uniform" && line.find('[') != std::string::npos) {
				tst::fail(__FILE__, __LINE__, "shader uses uniform array: " + line);
				break;
			}
		}
	}

	++tst::checks();
	if (text.find("for (") != std::string::npos)
		tst::fail(__FILE__, __LINE__, "shader uses 'for' loop (avoid)");

	++tst::checks();
	if (text.find("sign(") != std::string::npos)
		tst::fail(__FILE__, __LINE__, "shader uses sign() (avoid)");

	++tst::checks();
	if (text.find("length(") != std::string::npos)
		tst::fail(__FILE__, __LINE__, "shader uses length() (avoid)");
}
