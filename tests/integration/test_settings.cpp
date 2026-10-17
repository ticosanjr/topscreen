#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

#include <string>

#include "audio/ts-click-sound.hpp"
#include "core/keys.hpp"
#include "input/ts-input.hpp"
#include "ts-properties.hpp"
#include "ts-settings.hpp"

using namespace topscreen;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

/* Defaults do codigo == Config padrao, e defaults nao viram valor de usuario. */
TEST(settings_defaults_coincidem_com_config)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);

	CHECK(!obs_data_has_user_value(s, core::keys::kZoomAmount));
	CHECK(!obs_data_has_user_value(s, core::keys::kProfileCurrent));
	CHECK(obs_data_has_default_value(s, core::keys::kZoomAmount));

	core::Config c;
	config_load(s, &c);

	const core::Config d;
	CHECK_NEAR(c.zoom.amount, d.zoom.amount, 1e-9);
	CHECK_EQ(c.zoom.mode, d.zoom.mode);
	CHECK_EQ(c.click_left.color, d.click_left.color);
	CHECK_EQ(c.click_right.color, d.click_right.color);
	CHECK_EQ(c.bg.mode, d.bg.mode);
	CHECK(c.effects_enabled == d.effects_enabled);
	CHECK(c.cursor.enabled == d.cursor.enabled);

	obs_data_release(s);
}

/* Fonte antiga (sem nenhuma chave): cai nos defaults do codigo. */
TEST(settings_fonte_antiga_usa_defaults)
{
	obs_data_t *s = obs_data_create();

	core::Config c;
	c.zoom.amount = -1.0;
	config_load(s, &c);

	const core::Config d;
	CHECK_NEAR(c.zoom.amount, d.zoom.amount, 1e-9);
	CHECK_EQ(c.click_left.color, d.click_left.color);

	obs_data_release(s);
}

/* Store -> load preserva os campos (roundtrip). */
TEST(settings_store_load_roundtrip)
{
	core::Config a;
	a.effects_enabled = false;
	a.zoom.mode = static_cast<int>(core::ZoomMode::Off);
	a.zoom.amount = 2.5;
	a.click_left.color = 0xFF112233u;
	a.click_right.color = 0xFF445566u;
	a.bg.mode = static_cast<int>(core::BgMode::Vertical);
	a.bg.image_file = "wallpapers/ocean.jpg";
	a.keys.pos_x = 0.25;
	a.highlight.color = 0x6600E0FFu;
	a.cursor.size = 1.5;

	obs_data_t *s = obs_data_create();
	config_store(s, a);

	core::Config b;
	config_load(s, &b);

	CHECK(b.effects_enabled == false);
	CHECK_EQ(b.zoom.mode, a.zoom.mode);
	CHECK_NEAR(b.zoom.amount, 2.5, 1e-9);
	CHECK_EQ(b.click_left.color, a.click_left.color);
	CHECK_EQ(b.click_right.color, a.click_right.color);
	CHECK_EQ(b.bg.mode, a.bg.mode);
	CHECK_EQ(b.bg.image_file, a.bg.image_file);
	CHECK_NEAR(b.keys.pos_x, 0.25, 1e-9);
	CHECK_EQ(b.highlight.color, a.highlight.color);
	CHECK_NEAR(b.cursor.size, 1.5, 1e-9);

	obs_data_release(s);
}

/* Migracao da chave antiga bg_padding. */
TEST(settings_migra_bg_padding_antigo)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);
	obs_data_set_double(s, "bg_padding", 100.0);

	core::Config c;
	config_load(s, &c);
	CHECK_NEAR(c.bg.padding_x, 100.0, 1e-9);
	CHECK_NEAR(c.bg.padding_y, 100.0, 1e-9);

	obs_data_release(s);
}

/* Migracao dos defaults antigos de suavizacao do cursor. */
TEST(settings_migra_cursor_suave_antigo)
{
	obs_data_t *s = obs_data_create();
	config_defaults(s);
	obs_data_set_double(s, core::keys::kCursorSmooth, 0.7);
	obs_data_set_double(s, core::keys::kCursorSway, 0.3);

	core::Config c;
	config_load(s, &c);
	CHECK_NEAR(c.cursor.smooth, 0.0, 1e-9);
	CHECK_NEAR(c.cursor.sway, 0.0, 1e-9);

	obs_data_release(s);
}

/* Propriedades: grupo de perfis no topo + chaves principais presentes. */
TEST(properties_cria_grupos_esperados)
{
	obs_properties_t *p = properties_create(nullptr);
	CHECK(p != nullptr);
	CHECK(obs_properties_get(p, "grp_profiles") != nullptr);
	CHECK(obs_properties_get(p, core::keys::kEffectsEnabled) != nullptr);
	CHECK(obs_properties_get(p, "grp_bg") != nullptr);
	CHECK(obs_properties_get(p, "grp_cursor") != nullptr);
	CHECK(obs_properties_get(p, "grp_zoom") != nullptr);
	CHECK(obs_properties_get(p, "grp_keys") != nullptr);
	CHECK(obs_properties_get(p, "grp_highlight") != nullptr);
	obs_properties_destroy(p);
}

/* Input sem backend: disponivel=false, tier 0, sem bordas fantasmas. */
TEST(input_sem_backend_e_seguro)
{
	Input in;
	in.poll();
	CHECK(!in.available());
	CHECK_EQ(in.tier(), 0);
	in.poll();
	CHECK(!in.available());
}

/* Som: arquivo inexistente falha sem crash; play sem load e seguro. */
TEST(clicksound_falha_graciosa)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	ClickSound snd;
	CHECK(!snd.loaded());
	CHECK(!snd.load("sounds/arquivo-que-nao-existe.wav"));
	snd.play();
	CHECK(!snd.loaded());

	CHECK(snd.load("sounds/click-soft.wav"));
	CHECK(snd.loaded());
	CHECK(snd.wav().sample_rate > 0);
	/* Nao toca de verdade no teste (evita fork/exec na suite). */
	snd.release();
	CHECK(!snd.loaded());
}
