#pragma once

// "Fake libobs" for integration tests (without a real OBS).
//
// The plugin code is compiled against the REAL OBS headers and linked
// against these implementations. The public types are opaque, so we can
// define the structs here.

#include <obs-module.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct obs_data {
	std::map<std::string, bool> bools;
	std::map<std::string, long long> ints;
	std::map<std::string, double> doubles;
	std::map<std::string, std::string> strings;
	/* Defaults are kept separate from user values (as in real OBS):
	 * set_default_* does NOT create a user value. */
	std::map<std::string, bool> def_bools;
	std::map<std::string, long long> def_ints;
	std::map<std::string, double> def_doubles;
	std::map<std::string, std::string> def_strings;
	std::map<std::string, obs_data_t *> objs;
};

struct obs_property {
	std::string name;
};

struct obs_properties {
	std::map<std::string, obs_property *> items;
};

struct obs_source {
	const obs_source_info *info = nullptr;
	void *data = nullptr;
	obs_data_t *settings = nullptr;
	obs_source_t *parent = nullptr;
	uint32_t base_w = 0;
	uint32_t base_h = 0;
};

namespace fakeobs {

/** Sets the data directory resolved by obs_module_file(). */
void set_data_dir(const std::string &dir);

/** Creates a fake source (without registering). */
obs_source_t *make_source(const obs_source_info *info, obs_source_t *parent, uint32_t w, uint32_t h);

/** Last source registered by obs_register_source(). */
const obs_source_info *registered_source();

/** Index of the hotkey registered with this name (-1 if none). */
int find_hotkey(const char *name);

/** Triggers the hotkey at the index (pressed=true/false). */
void invoke_hotkey(int index, bool pressed);

int hotkey_count();

} // namespace fakeobs
