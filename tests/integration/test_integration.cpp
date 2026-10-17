#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

#include <cstdlib>
#include <string>

#if defined(_WIN32)
#define TS_SETENV(k, v) _putenv_s(k, v)
#define TS_UNSETENV(k) _putenv_s(k, "")
#else
#define TS_SETENV(k, v) ::setenv(k, v, 1)
#define TS_UNSETENV(k) ::unsetenv(k)
#endif

#include "core/keys.hpp"
#include "core/profiles.hpp"
#include "core/types.hpp"
#include "ts-filter.hpp"
#include "ts-registry.hpp"

using namespace topscreen;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

namespace {

/* Fonte-pai fake que expoe a propriedade "capture_cursor" (como uma Captura de Tela). */
obs_properties_t *parent_props(void *)
{
	obs_properties_t *p = obs_properties_create();
	obs_properties_add_bool(p, "capture_cursor", "Capture cursor");
	return p;
}

const obs_source_info parent_info = {
	.id = "fake_display_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_properties = parent_props,
};

obs_source_t *make_parent(uint32_t w, uint32_t h)
{
	obs_source_t *parent = fakeobs::make_source(&parent_info, nullptr, w, h);
	obs_data_set_bool(parent->settings, "capture_cursor", true);
	return parent;
}

void *create_filter(obs_source_t *filter)
{
	/* No OBS real, o obs_data passado ao create E o da propria fonte. */
	ts_filter_source_info.get_defaults(filter->settings);
	return ts_filter_source_info.create(filter->settings, filter);
}

/* Simula a lista de perfis nas Propriedades: grava profile_current e chama update. */
void set_profile_from_properties(obs_source_t *filter, void *ctx, int index)
{
	obs_data_set_int(filter->settings, core::keys::kProfileCurrent, index);
	ts_filter_source_info.update(ctx, filter->settings);
}

} // namespace

TEST(integration_ciclo_de_vida_do_filtro)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	CHECK_EQ(ctrl->preset_count(), 3);

	/* Render define as dimensoes alvo e, se o efeito carregou, desativa o
	 * cursor nativo do pai (T2.1). */
	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "capture_cursor"));

	for (int i = 0; i < 10; ++i)
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	REQUIRE(fakeobs::hotkey_count() >= 12);

	/* Toggle de efeitos via hotkey. */
	const int toggle = fakeobs::find_hotkey("TopScreen.ToggleEffects");
	REQUIRE(toggle >= 0);
	fakeobs::invoke_hotkey(toggle, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->effects_enabled());

	/* Zoom via hotkey nao deve derrubar nada. */
	const int zin = fakeobs::find_hotkey("TopScreen.ZoomIn");
	REQUIRE(zin >= 0);
	fakeobs::invoke_hotkey(zin, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Preset via FilterControl (thread da UI -> aplicado no tick). */
	ctrl->apply_preset(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_preset(), 1);
	/* tutorial.json usa fundo solido (bg_mode = 0). */
	CHECK_EQ(static_cast<int>(obs_data_get_int(filter->settings, core::keys::kBgMode)), 0);

	ts_filter_source_info.destroy(ctx);
	CHECK(registry_first() == nullptr);
}

TEST(integration_todos_os_presets_e_toggle_background)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	for (int i = 0; i < ctrl->preset_count(); ++i) {
		ctrl->apply_preset(i);
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		CHECK_EQ(ctrl->current_preset(), i);
		CHECK(!ctrl->preset_name(i).empty());
	}

	const int tg = fakeobs::find_hotkey("TopScreen.ToggleBackground");
	REQUIRE(tg >= 0);
	const bool before = obs_data_get_bool(filter->settings, core::keys::kBgEnabled);
	fakeobs::invoke_hotkey(tg, true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!obs_data_get_bool(filter->settings, core::keys::kBgEnabled) != !before);

	ts_filter_source_info.destroy(ctx);
}

TEST(integration_hotkeys_todas_as_acoes)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1280, 720);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	const char *names[] = {"TopScreen.ToggleEffects", "TopScreen.ToggleBackground",
			       "TopScreen.ZoomIn",	      "TopScreen.ZoomOut",
			       "TopScreen.ResetZoom",	      "TopScreen.NextPreset",
			       "TopScreen.PrevPreset",	      "TopScreen.ProfileA",
			       "TopScreen.ProfileB",	      "TopScreen.ProfileC",
			       "TopScreen.ProfileD",	      "TopScreen.ProfileSave"};
	for (const char *n : names)
		REQUIRE(fakeobs::find_hotkey(n) >= 0);

	/* pressed=false nao faz nada (early-return do callback). */
	const bool fx0 = ctrl->effects_enabled();
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ToggleEffects"), false);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ProfileC"), false);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->effects_enabled() == fx0);
	CHECK_EQ(ctrl->current_profile(), 0);

	/* pressed=true: cada acao chega ao filtro. */
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ToggleEffects"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->effects_enabled() == !fx0);

	const bool bg0 = obs_data_get_bool(filter->settings, core::keys::kBgEnabled);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ToggleBackground"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_bool(filter->settings, core::keys::kBgEnabled) == !bg0);

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ZoomIn"), true);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ZoomOut"), true);
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ResetZoom"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.NextPreset"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->current_preset() >= 0 && ctrl->current_preset() < ctrl->preset_count());
	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.PrevPreset"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->current_preset() >= 0 && ctrl->current_preset() < ctrl->preset_count());

	const char *profiles[] = {"TopScreen.ProfileA", "TopScreen.ProfileB", "TopScreen.ProfileC",
				   "TopScreen.ProfileD"};
	for (int i = 0; i < 4; ++i) {
		fakeobs::invoke_hotkey(fakeobs::find_hotkey(profiles[i]), true);
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		CHECK_EQ(ctrl->current_profile(), i);
	}

	fakeobs::invoke_hotkey(fakeobs::find_hotkey("TopScreen.ProfileSave"), true);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(obs_data_get_obj(filter->settings, core::keys::kProfileSlots[ctrl->current_profile()]) !=
	      nullptr);

	ts_filter_source_info.destroy(ctx);
}

TEST(integration_overlay_de_teclas)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	/* Injeta teclas pelo gancho de depuracao (sem back-end de input). */
	TS_SETENV("TOPSCREEN_DEBUG_KEY", "Ctrl");

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	ts_filter_source_info.video_render(ctx, nullptr);

	/* Roda frames suficientes para o gancho disparar e o overlay ser desenhado. */
	for (int i = 0; i < 130; ++i) {
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		ts_filter_source_info.video_render(ctx, nullptr);
	}

	TS_UNSETENV("TOPSCREEN_DEBUG_KEY");

	ts_filter_source_info.destroy(ctx);
	CHECK(registry_first() == nullptr);
}

TEST(integration_perfis_salvar_e_trocar)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);
	CHECK_EQ(ctrl->profile_count(), 4);

	/* Perfil A: estado atual (efeitos ligados) -> salva. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_profile(), 0);
	CHECK(ctrl->effects_enabled());
	ctrl->save_profile();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Perfil B: efeitos desligados -> salva. */
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ctrl->set_effects_enabled(false);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(!ctrl->effects_enabled());
	ctrl->save_profile();
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Volta para A: efeitos ligados de novo. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_profile(), 0);
	CHECK(ctrl->effects_enabled());

	/* E para B: desligados. */
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(ctrl->current_profile(), 1);
	CHECK(!ctrl->effects_enabled());

	/* Perfil valido fora da faixa nao deve quebrar. */
	ctrl->select_profile(9);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK(ctrl->current_profile() >= 0 && ctrl->current_profile() < 4);

	ts_filter_source_info.destroy(ctx);
	CHECK(registry_first() == nullptr);
}

TEST(integration_perfis_semeados_restauram)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	FilterControl *ctrl = registry_first();
	REQUIRE(ctrl != nullptr);

	/* Perfil A (padrao): zoom 1.6 e clique esquerdo branco. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.6, 1e-6);

	/* Perfil B: zoom 2.0 e clique esquerdo VERDE (semeado). */
	ctrl->select_profile(1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 2.0, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00FF00));

	/* Voltar para A tem que restaurar TUDO. */
	ctrl->select_profile(0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.6, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFFFFFF));

	ts_filter_source_info.destroy(ctx);
}

TEST(integration_perfil_da_lista_restaura_tudo)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Perfil B (semeado): zoom 2.0 + clique esquerdo verde. */
	set_profile_from_properties(filter, ctx, 1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 2.0, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00FF00));

	/* As chaves principais tem que ir junto, senao o proximo reload desfaz o perfil. */
	CHECK(obs_data_has_user_value(filter->settings, core::keys::kZoomAmount));

	/* Mexer em OUTRA propriedade nao pode desfazer o perfil ativo. */
	obs_data_set_bool(filter->settings, core::keys::kHighlightEnabled, false);
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 2.0, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00FF00));

	/* Perfil C (semeado): roxo/ciano + zoom 1.4. */
	set_profile_from_properties(filter, ctx, 2);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_NEAR(obs_data_get_double(filter->settings, core::keys::kZoomAmount), 1.4, 1e-6);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFF5C7C));

	ts_filter_source_info.destroy(ctx);
	CHECK(registry_first() == nullptr);
}

TEST(integration_perfil_guarda_edicao_do_usuario)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Vai para o perfil D (zoom desligado). */
	set_profile_from_properties(filter, ctx, 3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kZoomMode),
		 static_cast<long long>(core::ZoomMode::Off));

	/* Usuario muda a cor do clique esquerdo (valor que nao existe em nenhum seed). */
	obs_data_set_int(filter->settings, core::keys::kClickLeftColor,
			 static_cast<long long>(0xFF112233));
	ts_filter_source_info.update(ctx, filter->settings);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);

	/* Troca para A e volta para D: a edicao tem que ter ficado no perfil D. */
	set_profile_from_properties(filter, ctx, 0);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFFFFFFFF));

	set_profile_from_properties(filter, ctx, 3);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	CHECK_EQ(obs_data_get_int(filter->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF112233));

	ts_filter_source_info.destroy(ctx);
}

TEST(integration_perfil_escolhido_sobrevive_ao_reinicio)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	obs_source_t *parent = make_parent(1920, 1080);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);

	/* Escolhe o perfil B e "fecha o OBS". */
	set_profile_from_properties(filter, ctx, 1);
	ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
	ts_filter_source_info.destroy(ctx);

	/* "Reabre o OBS": a colecao salva tem profile_current + os slots. */
	obs_data_t saved = *filter->settings;
	obs_source_t *f2 = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	*f2->settings = saved;
	void *ctx2 = ts_filter_source_info.create(f2->settings, f2);
	REQUIRE(ctx2 != nullptr);
	ts_filter_source_info.video_tick(ctx2, 1.0f / 60.0f);

	/* O estado tem que ser o do perfil B (nao o padrao). */
	CHECK_NEAR(obs_data_get_double(f2->settings, core::keys::kZoomAmount), 2.0, 1e-6);
	CHECK_EQ(obs_data_get_int(f2->settings, core::keys::kClickLeftColor),
		 static_cast<long long>(0xFF00FF00));

	ts_filter_source_info.destroy(ctx2);
}

TEST(integration_render_nao_crasha_sem_efeito)
{
	fakeobs::set_data_dir("/diretorio/inexistente");

	obs_source_t *parent = make_parent(640, 480);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);

	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);

	/* Sem efeito (data dir invalido) => passthrough, sem crash. */
	for (int i = 0; i < 5; ++i) {
		ts_filter_source_info.video_tick(ctx, 1.0f / 60.0f);
		ts_filter_source_info.video_render(ctx, nullptr);
	}

	/* Nao tentou mexer no cursor do pai (render saiu cedo). */
	CHECK(obs_data_get_bool(parent->settings, "capture_cursor"));

	ts_filter_source_info.destroy(ctx);
}

/* Fonte-pai fake que usa "show_cursor" (como o xshm_input do Linux). */
namespace {
obs_properties_t *parent_props_show_cursor(void *)
{
	obs_properties_t *p = obs_properties_create();
	obs_properties_add_bool(p, "show_cursor", "Show cursor");
	return p;
}
const obs_source_info parent_info2 = {
	.id = "fake_xshm_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_properties = parent_props_show_cursor,
};
} // namespace

TEST(integration_desativa_show_cursor_do_pai)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	obs_source_t *parent = fakeobs::make_source(&parent_info2, nullptr, 1920, 1080);
	obs_data_set_bool(parent->settings, "show_cursor", true);
	obs_source_t *filter = fakeobs::make_source(&ts_filter_source_info, parent, 0, 0);
	void *ctx = create_filter(filter);
	REQUIRE(ctx != nullptr);
	ts_filter_source_info.video_render(ctx, nullptr);
	CHECK(!obs_data_get_bool(parent->settings, "show_cursor"));
	ts_filter_source_info.destroy(ctx);
}
