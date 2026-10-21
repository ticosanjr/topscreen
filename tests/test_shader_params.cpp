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
		"bg_transparent", "bg_tex_aspect",  "bg_blur",       "bg_blur_type",  "bg_keep_aspect",        "shadow_alpha",  "shadow_blur",
		"shadow_offset",  "cursor_enabled", "cursor_pos",    "cursor_scale",
		"cursor_opacity", "cursor_rotation", "cursor_shadow", "click_count",
		"cursor_tex",     "cursor_tex_enabled", "cursor_cell_off", "cursor_cell_scale",
		"cursor_hotspot",
		"zoom_blur",      "spotlight_enabled", "spotlight_amount",
		"spotlight_shape", "spotlight_radius", "spotlight_corner",
		"spotlight_dim", "spotlight_border_enabled", "spotlight_border_color",
		"spotlight_border_width", "spotlight_shadow_enabled", "spotlight_shadow_color",
		"spotlight_shadow_alpha", "spotlight_shadow_blur", "spotlight_shadow_offset",
		"spotlight_blur_enabled", "spotlight_blur", "spotlight_zoom",
		"spotlight_focus", "spotlight_drop",
		"highlight_active", "highlight_rect", "highlight_color", "highlight_alpha",
		"click0_pos",     "click0_prog",    "click0_size",   "click0_color",
		"click0_type",    "click1_pos",     "click1_prog",   "click1_size",
		"click1_color",   "click1_type",    "click2_pos",    "click2_prog",
		"click2_size",    "click2_color",   "click2_type",   "click3_pos",
		"click3_prog",    "click3_size",    "click3_color",  "click3_type",
		"line_count",     "line_width",     "line_p0",       "line_p1",
		"line_p2",        "line_p3",        "line_p4",       "line_p5",
		"line_p6",        "line_p7",        "line_p8",       "line_p9",
		"line_p10",       "line_p11",       "line_p12",      "line_p13",
		"line_p14",       "line_p15",
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

/*
 * The CPU decimates the freehand trail to 16 points (line_p0..line_p15, see
 * kShaderLineSlots) and relies on the shader pairing every consecutive couple.
 * A shader that lost a segment would silently drop part of the stroke - the
 * exact "line mode does nothing" class of bug - so the pairing is pinned here.
 */
TEST(shader_line_segments_pair_consecutive_points)
{
	const std::string text = read_effect();
	REQUIRE(!text.empty());

	const int slots = 16;
	for (int i = 1; i < slots; ++i) {
		const std::string guard = "line_count > " + std::to_string(i) + ".5";
		++tst::checks();
		if (text.find(guard) == std::string::npos)
			tst::fail(__FILE__, __LINE__, "missing line segment guard: " + guard);

		const std::string a = "content_to_pixel(line_p" + std::to_string(i - 1) + ")";
		const std::string b = "content_to_pixel(line_p" + std::to_string(i) + ")";
		++tst::checks();
		if (text.find(a) == std::string::npos || text.find(b) == std::string::npos)
			tst::fail(__FILE__, __LINE__,
				  "line segment " + std::to_string(i) + " does not pair line_p" +
					  std::to_string(i - 1) + " with line_p" + std::to_string(i));
	}

	/* The CPU reads these three by name in the compositor. */
	for (const char *name : {"line_count", "line_width", "line_p15"}) {
		const std::string decl = std::string(name) + ";";
		bool declared = false;
		std::istringstream in(text);
		std::string line;
		while (std::getline(in, line)) {
			if (line.rfind("uniform ", 0) == 0 && line.find(decl) != std::string::npos) {
				declared = true;
				break;
			}
		}
		++tst::checks();
		if (!declared)
			tst::fail(__FILE__, __LINE__, std::string("line uniform missing: ") + name);
	}

	++tst::checks();
	if (text.find("seg_dist(") == std::string::npos)
		tst::fail(__FILE__, __LINE__, "line segments do not use the pixel-distance helper");
}
