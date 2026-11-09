#pragma once

// "Fake libobs" for integration tests (without a real OBS).
//
// The plugin code is compiled against the REAL OBS headers and linked
// against these implementations. The public types are opaque, so we can
// define the structs here.

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct obs_data_array {
	std::vector<obs_data_t *> items;
	int refs = 1;
};

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
	std::map<std::string, obs_data_array_t *> arrays;
	int refs = 1;
};

struct obs_property {
	std::string name;
	bool visible = true;
	obs_property_modified_t modified = nullptr;
};

struct obs_properties {
	std::map<std::string, obs_property *> items;
	/* Nested groups, so obs_properties_get() can find them like the real one. */
	std::map<std::string, obs_properties_t *> groups;
};

/** Visibility flag (mirrors obs_property_set_visible). */
namespace fakeobs {
bool property_visible(obs_property_t *prop);
/** Runs the stored modified callback, as OBS does when the UI changes. */
bool invoke_property_modified(obs_properties_t *props, obs_property_t *prop,
			      obs_data_t *settings);
} // namespace fakeobs

struct obs_source {
	const obs_source_info *info = nullptr;
	void *data = nullptr;
	obs_data_t *settings = nullptr;
	obs_source_t *parent = nullptr;
	std::string name;
	uint32_t base_w = 0;
	uint32_t base_h = 0;
};

namespace fakeobs {

/** Sets the data directory resolved by obs_module_file(). */
void set_data_dir(const std::string &dir);

/** Creates a fake source (without registering). */
obs_source_t *make_source(const obs_source_info *info, obs_source_t *parent, uint32_t w, uint32_t h);

/** Associates a plugin context with its fake source so it can be destroyed at exit. */
void attach_filter_ctx(obs_source_t *source, void *ctx);

/** Marks a plugin context as already destroyed (safe to call with any pointer). */
void detach_filter_ctx(void *ctx);

/** Destroys every remaining plugin context/source created by the tests. */
void cleanup();


/** Sets the name returned by obs_source_get_name() (default: "fake-parent"). */
void set_source_name(obs_source_t *source, const std::string &name);

/** Last source registered by obs_register_source(). */
const obs_source_info *registered_source();

/** Index of the hotkey registered with this name (-1 if none). */
int find_hotkey(const char *name);

/** Triggers the hotkey at the index (pressed=true/false). */
void invoke_hotkey(int index, bool pressed);

int hotkey_count();
/** True when every hotkey registered from `index` on has a callback and is active. */
bool hotkeys_active_from(int index);

/** Latest registered bindings for a hotkey name (empty when absent). */
const std::vector<obs_key_combination_t> &hotkey_bindings(const char *name);

/** Profile config value used by the frontend hotkey loader. */
void set_profile_config_string(const char *section, const char *name, const char *value);

/** Simulates an OBS frontend event (e.g. profile changed). */
void fire_frontend_event(enum obs_frontend_event event);

/** Reads a value from the fake OBS profile config. */
std::string profile_config_string(const char *section, const char *name);

/** Records a uniform upload (used to catch D3D11's "parameter not set"). */
void note_uniform_set(const std::string &name);
/** Names of every uniform uploaded since the last clear_uniforms(). */
const std::vector<std::string> &uploaded_uniforms();
void clear_uniforms();

/* ---- graphics counters (text batching tests) ---- */
void reset_graphics_counters();
int graphics_draw_calls();
int graphics_sprite_draw_calls();
int graphics_render_starts();
int graphics_vertex_calls();
int graphics_color_calls();
/** Makes gs_render_save() fail, exercising the per-glyph fallback. */
void set_fail_vertex_buffers(bool fail);

/* ---- properties refresh counter (profile/preset UI synchronization) ---- */
void reset_property_update_calls();
int property_update_calls();

} // namespace fakeobs
