#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <obs-module.h>

#include <cstdlib>
#include <string>

#include "core/keys.hpp"
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
	obs_data_t *settings = obs_data_create();
	ts_filter_source_info.get_defaults(settings);
	return ts_filter_source_info.create(settings, filter);
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

	REQUIRE(fakeobs::hotkey_count() >= 7);

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

TEST(integration_overlay_de_teclas)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	/* Injeta teclas pelo gancho de depuracao (sem back-end de input). */
	::setenv("TOPSCREEN_DEBUG_KEY", "Ctrl", 1);

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

	::unsetenv("TOPSCREEN_DEBUG_KEY");

	ts_filter_source_info.destroy(ctx);
	CHECK(registry_first() == nullptr);
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
