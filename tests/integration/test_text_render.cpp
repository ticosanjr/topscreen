#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <string>

#include "core/key_overlay.hpp"
#include "render/ts-text.hpp"

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

using namespace topscreen;

namespace {

core::KeyRender make_key(const char *label)
{
	core::KeyRender key;
	key.label.assign(label);
	return key;
}

} // namespace

TEST(integration_text_batches_glyph_draws)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	TextRenderer text;
	REQUIRE(text.load("fonts/DejaVuSans-Bold.ttf", "effects/text.effect"));

	core::KeyRender keys[2] = {make_key("A"), make_key("Backspace")};
	core::KeyOverlayConfig cfg;
	cfg.band = false; /* isolate the text draws from the band sprite */
	cfg.shadow = true;

	fakeobs::reset_graphics_counters();
	text.draw(keys, 2, cfg, 1920, 1080);
	CHECK_EQ(fakeobs::graphics_render_starts(), 1);
	CHECK_EQ(fakeobs::graphics_draw_calls(), 1); /* shadow + text in one buffer */
	CHECK_EQ(fakeobs::graphics_sprite_draw_calls(), 0);
	/* Every vertex must carry a color: libobs' gs_color4v() is a no-op and
	 * leaves the text vertex buffer without the COLOR input. */
	CHECK(fakeobs::graphics_vertex_calls() > 0);
	CHECK_EQ(fakeobs::graphics_color_calls(), fakeobs::graphics_vertex_calls());

	/* Same labels: the cached layout/vertex buffer is reused. */
	fakeobs::reset_graphics_counters();
	text.draw(keys, 2, cfg, 1920, 1080);
	CHECK_EQ(fakeobs::graphics_render_starts(), 0);
	CHECK_EQ(fakeobs::graphics_draw_calls(), 1);
	CHECK_EQ(fakeobs::graphics_sprite_draw_calls(), 0);

	/* No shadow: the shadow quads leave the cached buffer, still one draw. */
	cfg.shadow = false;
	fakeobs::reset_graphics_counters();
	text.draw(keys, 2, cfg, 1920, 1080);
	CHECK_EQ(fakeobs::graphics_render_starts(), 1);
	CHECK_EQ(fakeobs::graphics_draw_calls(), 1);
	CHECK_EQ(fakeobs::graphics_sprite_draw_calls(), 0);

	/* A new label rebuilds the vertex buffer once. */
	keys[1] = make_key("Enter");
	cfg.shadow = true;
	fakeobs::reset_graphics_counters();
	text.draw(keys, 2, cfg, 1920, 1080);
	CHECK_EQ(fakeobs::graphics_render_starts(), 1);
	CHECK_EQ(fakeobs::graphics_draw_calls(), 1);
	CHECK_EQ(fakeobs::graphics_sprite_draw_calls(), 0);
}

TEST(integration_text_falls_back_to_glyph_sprites)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);

	TextRenderer text;
	REQUIRE(text.load("fonts/DejaVuSans-Bold.ttf", "effects/text.effect"));

	core::KeyRender keys[2] = {make_key("A"), make_key("Backspace")};
	core::KeyOverlayConfig cfg;
	cfg.band = false;
	cfg.shadow = true;

	/* Simulate a vertex-buffer allocation failure: the renderer must still
	 * draw every glyph through the old sprite path. */
	fakeobs::set_fail_vertex_buffers(true);
	fakeobs::reset_graphics_counters();
	text.draw(keys, 2, cfg, 1920, 1080);
	CHECK_EQ(fakeobs::graphics_draw_calls(), 0);
	CHECK(fakeobs::graphics_sprite_draw_calls() > 0);

	fakeobs::set_fail_vertex_buffers(false);

	/* A text change rebuilds and returns to the batched path. */
	keys[1] = make_key("Tab");
	fakeobs::reset_graphics_counters();
	text.draw(keys, 2, cfg, 1920, 1080);
	CHECK_EQ(fakeobs::graphics_render_starts(), 1);
	CHECK_EQ(fakeobs::graphics_draw_calls(), 1);
	CHECK_EQ(fakeobs::graphics_sprite_draw_calls(), 0);
}
