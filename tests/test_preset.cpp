#include "core/preset.hpp"
#include "test_framework.hpp"

#include <fstream>
#include <sstream>
#include <string>

using namespace topscreen::core;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

namespace {

std::string read_file(const std::string &path, bool *ok)
{
	std::ifstream in(path);
	if (!in) {
		*ok = false;
		return {};
	}
	std::ostringstream ss;
	ss << in.rdbuf();
	*ok = true;
	return ss.str();
}

} // namespace

TEST(preset_from_json_e_apply)
{
	const std::string text = R"({
        "name": "Teste",
        "description": "preset de teste",
        "settings": {
            "effects_enabled": true,
            "bg_enabled": true,
            "bg_mode": 1,
            "bg_color_a": 4278190080,
            "bg_padding_x": 32,
            "bg_corner_radius": 8,
            "cursor_enabled": false,
            "cursor_size": 2.0,
            "zoom_amount": 2.5
        }
    })";

	Preset p;
	std::string err;
	REQUIRE(Preset::from_json(text, &p, &err));
	CHECK_EQ(p.name, std::string("Teste"));

	Config cfg;
	cfg.bg.padding_x = 999.0; /* sera sobrescrito */
	REQUIRE(p.apply(&cfg, &err));

	CHECK_EQ(cfg.bg.mode, 1);
	CHECK_NEAR(cfg.bg.padding_x, 32.0, 1e-9);
	CHECK_NEAR(cfg.bg.corner_radius, 8.0, 1e-9);
	CHECK_EQ(cfg.bg.color_a, 4278190080u);
	CHECK(!cfg.cursor.enabled);
	CHECK_NEAR(cfg.cursor.size, 2.0, 1e-9);
	CHECK_NEAR(cfg.zoom.amount, 2.5, 1e-9);
}

TEST(preset_valores_ausentes_mantidos)
{
	const std::string text = R"({"name":"X","settings":{"bg_padding_x":10}})";
	Preset p;
	std::string err;
	REQUIRE(Preset::from_json(text, &p, &err));

	Config cfg;
	cfg.bg.corner_radius = 42.0;
	cfg.zoom.amount = 3.0;
	REQUIRE(p.apply(&cfg, &err));

	CHECK_NEAR(cfg.bg.padding_x, 10.0, 1e-9);
	CHECK_NEAR(cfg.bg.corner_radius, 42.0, 1e-9); /* nao estava no preset */
	CHECK_NEAR(cfg.zoom.amount, 3.0, 1e-9);
}

TEST(preset_sem_settings_falha)
{
	Preset p;
	std::string err;
	CHECK(!Preset::from_json(R"({"name":"X"})", &p, &err));
	CHECK(!err.empty());
}

TEST(preset_arquivos_reais_parseiam)
{
	const char *names[] = {"demo.json", "tutorial.json", "apresentacao.json"};
	for (const char *n : names) {
		const std::string path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/presets/" + n;
		bool ok = false;
		const std::string text = read_file(path, &ok);
		REQUIRE(ok);

		Preset p;
		std::string err;
		++tst::checks();
		if (!Preset::from_json(text, &p, &err))
			tst::fail(__FILE__, __LINE__, "preset invalido: " + path + " -> " + err);

		Config cfg;
		++tst::checks();
		if (!p.apply(&cfg, &err))
			tst::fail(__FILE__, __LINE__, "apply falhou: " + path + " -> " + err);

		CHECK(!p.name.empty());
	}
}
