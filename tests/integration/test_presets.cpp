#include "harness/fake_libobs.hpp"
#include "test_framework.hpp"

#include <filesystem>
#include <string>

#include "core/types.hpp"
#include "ts-presets.hpp"

using namespace topscreen;

#ifndef TOPSCREEN_TEST_DATA_DIR
#define TOPSCREEN_TEST_DATA_DIR "data"
#endif

/* User looks are single JSON files in the writable looks directory. */
TEST(presets_user_look_save_apply_delete)
{
	fakeobs::set_data_dir(TOPSCREEN_TEST_DATA_DIR);
	const std::filesystem::path dir =
		std::filesystem::temp_directory_path() / "topscreen-looks-test";
	std::filesystem::remove_all(dir);
	fakeobs::set_config_dir(dir.string());

	PresetStore store;
	store.set_user_dir(dir.string());
	CHECK_EQ(store.load(), 4);

	core::Config cfg;
	cfg.bg.mode = static_cast<int>(core::BgMode::Solid);
	cfg.zoom.amount = 2.25;
	std::string error;
	const int index = store.save_user("My Look", cfg, &error);
	REQUIRE(index >= 0);
	CHECK(store.is_user(index));
	CHECK_EQ(store.name(index), std::string("My Look"));
	CHECK(std::filesystem::exists(dir / "My_Look.json"));

	core::Config applied;
	CHECK(store.apply(index, &applied));
	CHECK_EQ(applied.bg.mode, static_cast<int>(core::BgMode::Solid));
	CHECK_NEAR(applied.zoom.amount, 2.25, 1e-9);

	/* A fresh load discovers the file. */
	CHECK_EQ(store.load(), 5);
	int found = -1;
	for (int i = 0; i < store.count(); ++i) {
		if (store.name(i) == "My Look")
			found = i;
	}
	REQUIRE(found >= 0);
	CHECK(store.is_user(found));

	CHECK(store.delete_user(found, &error));
	CHECK_EQ(store.count(), 4);
	CHECK(!std::filesystem::exists(dir / "My_Look.json"));

	/* Bundled looks cannot be deleted. */
	CHECK(!store.delete_user(0, &error));

	std::filesystem::remove_all(dir);
}
