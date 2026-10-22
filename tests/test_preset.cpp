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

TEST(preset_from_json_and_apply)
{
	const std::string text = R"({
        "name": "Test",
        "description": "test preset",
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
	CHECK_EQ(p.name, std::string("Test"));

	Config cfg;
	cfg.bg.padding_x = 999.0; /* will be overwritten */
	REQUIRE(p.apply(&cfg, &err));

	CHECK_EQ(cfg.bg.mode, 1);
	CHECK_NEAR(cfg.bg.padding_x, 32.0, 1e-9);
	CHECK_NEAR(cfg.bg.corner_radius, 8.0, 1e-9);
	CHECK_EQ(cfg.bg.color_a, 4278190080u);
	CHECK(!cfg.cursor.enabled);
	CHECK_NEAR(cfg.cursor.size, 2.0, 1e-9);
	CHECK_NEAR(cfg.zoom.amount, 2.5, 1e-9);
}

TEST(preset_legacy_background_modes_are_migrated)
{
	auto apply = [](const char *json, Config *cfg) {
		Preset p;
		std::string err;
		REQUIRE(Preset::from_json(json, &p, &err));
		REQUIRE(p.apply(cfg, &err));
	};

	Config vertical;
	vertical.bg.gradient_angle = 12.0;
	apply(R"({"name":"V","settings":{"bg_mode":1}})", &vertical);
	CHECK_EQ(vertical.bg.mode, static_cast<int>(BgMode::Gradient));
	CHECK_NEAR(vertical.bg.gradient_angle, 90.0, 1e-9);

	Config diagonal;
	diagonal.bg.gradient_angle = 12.0;
	apply(R"({"name":"D","settings":{"bg_mode":2}})", &diagonal);
	CHECK_EQ(diagonal.bg.mode, static_cast<int>(BgMode::Gradient));
	CHECK_NEAR(diagonal.bg.gradient_angle, 45.0, 1e-9);

	Config wallpaper;
	apply(R"({"name":"W","settings":{"bg_mode":3}})", &wallpaper);
	CHECK_EQ(wallpaper.bg.mode, static_cast<int>(BgMode::Wallpaper));

	/* bg_type is authoritative when both keys are present. */
	Config modern;
	modern.bg.gradient_angle = 12.0;
	apply(R"({"name":"M","settings":{"bg_mode":3,"bg_type":0}})", &modern);
	CHECK_EQ(modern.bg.mode, static_cast<int>(BgMode::Solid));
	CHECK_NEAR(modern.bg.gradient_angle, 12.0, 1e-9);
}

TEST(preset_real_files_use_modern_background_key)
{
	const char *names[] = {"demo.json", "tutorial.json", "presentation.json"};
	for (const char *n : names) {
		const std::string path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/presets/" + n;
		bool ok = false;
		const std::string text = read_file(path, &ok);
		REQUIRE(ok);
		CHECK(text.find("\"bg_type\"") != std::string::npos);
		CHECK(text.find("\"bg_mode\"") == std::string::npos);
	}
}

TEST(preset_missing_values_kept)
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
	CHECK_NEAR(cfg.bg.corner_radius, 42.0, 1e-9); /* was not in the preset */
	CHECK_NEAR(cfg.zoom.amount, 3.0, 1e-9);
}

TEST(preset_without_settings_fails)
{
	Preset p;
	std::string err;
	CHECK(!Preset::from_json(R"({"name":"X"})", &p, &err));
	CHECK(!err.empty());
}

TEST(preset_legacy_click_sound_file_applies_to_both_sides)
{
	const std::string text = R"({
        "name": "Legacy",
        "description": "legacy single sound file",
        "settings": {
            "click_sound_file": "sounds/click-tick.wav"
        }
    })";

	Preset p;
	std::string err;
	REQUIRE(Preset::from_json(text, &p, &err));

	Config cfg;
	REQUIRE(p.apply(&cfg, &err));
	CHECK_EQ(cfg.click_sound.file_left, std::string("sounds/click-tick.wav"));
	CHECK_EQ(cfg.click_sound.file_right, std::string("sounds/click-tick.wav"));
}

TEST(preset_real_files_parse)
{
	const char *names[] = {"demo.json", "tutorial.json", "presentation.json"};
	for (const char *n : names) {
		const std::string path = std::string(TOPSCREEN_TEST_DATA_DIR) + "/presets/" + n;
		bool ok = false;
		const std::string text = read_file(path, &ok);
		REQUIRE(ok);

		Preset p;
		std::string err;
		++tst::checks();
		if (!Preset::from_json(text, &p, &err))
			tst::fail(__FILE__, __LINE__, "invalid preset: " + path + " -> " + err);

		Config cfg;
		++tst::checks();
		if (!p.apply(&cfg, &err))
			tst::fail(__FILE__, __LINE__, "apply failed: " + path + " -> " + err);

		CHECK(!p.name.empty());
	}
}
