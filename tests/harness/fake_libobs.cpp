#include "fake_libobs.hpp"

#include <obs-frontend-api.h>
#include <util/config-file.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <chrono>
#include <memory>
#include <unordered_map>

#include "core/json.hpp"

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
namespace {

std::string g_data_dir;
std::string g_current_scene_name = "Scene";
const obs_source_info *g_registered = nullptr;
std::vector<obs_source_t *> g_sources;
std::unordered_map<void *, obs_source_t *> g_filter_ctx;

struct RegisteredHotkey {
	std::string name;
	obs_hotkey_func func = nullptr;
	void *data = nullptr;
	bool active = true;
	std::vector<obs_key_combination_t> bindings;
};
std::vector<RegisteredHotkey> g_hotkeys;
std::map<std::string, std::string> g_profile_config;
obs_frontend_event_cb g_frontend_callback = nullptr;
void *g_frontend_callback_data = nullptr;

void cleanup_sources()
{
	/* Tests create filters and do not always destroy them. Destroying the
	 * remaining plugin contexts here keeps the leak sanitizer meaningful. */
	for (auto &entry : g_filter_ctx) {
		obs_source_t *source = entry.second;
		if (source && source->info && source->data && source->info->destroy)
			source->info->destroy(source->data);
	}
	g_filter_ctx.clear();

	for (obs_source_t *source : g_sources) {
		if (!source)
			continue;
		if (source->settings)
			obs_data_release(source->settings);
		delete source;
	}
	g_sources.clear();
	g_hotkeys.clear();
	g_profile_config.clear();
}

char g_effect_dummy = 0;
char g_technique_dummy = 0;
char g_texture_dummy = 0;
char g_vertex_dummy = 0;

int g_draw_calls = 0;
int g_property_updates = 0;
int g_sprite_draw_calls = 0;
int g_render_starts = 0;
int g_vertex_calls = 0;
int g_color_calls = 0;
bool g_fail_vertex_buffers = false;

obs_property_t *add_prop(obs_properties_t *props, const char *name)
{
	auto *p = new obs_property();
	p->name = name ? name : "";
	props->items[p->name] = p;
	return p;
}

} // namespace

namespace fakeobs {

void cleanup()
{
	cleanup_sources();
}

void set_data_dir(const std::string &dir)
{
	g_data_dir = dir;
}

void set_current_scene_name(const std::string &name)
{
	g_current_scene_name = name;
}

obs_source_t *make_source(const obs_source_info *info, obs_source_t *parent, uint32_t w, uint32_t h)
{
	auto *s = new obs_source();
	s->info = info;
	s->parent = parent;
	s->base_w = w;
	s->base_h = h;
	s->settings = obs_data_create();
	g_sources.push_back(s);
	return s;
}

void attach_filter_ctx(obs_source_t *source, void *ctx)
{
	if (!source || !ctx)
		return;
	source->data = ctx;
	g_filter_ctx[ctx] = source;
}

void detach_filter_ctx(void *ctx)
{
	if (!ctx)
		return;
	auto it = g_filter_ctx.find(ctx);
	if (it == g_filter_ctx.end())
		return;
	if (it->second)
		it->second->data = nullptr;
	g_filter_ctx.erase(it);
}

void set_source_name(obs_source_t *source, const std::string &name)
{
	if (source)
		source->name = name;
}

const obs_source_info *registered_source()
{
	return g_registered;
}

int find_hotkey(const char *name)
{
	for (size_t i = 0; i < g_hotkeys.size(); ++i)
		if (g_hotkeys[i].name == name)
			return static_cast<int>(i);
	return -1;
}

void invoke_hotkey(int index, bool pressed)
{
	if (index < 0 || index >= static_cast<int>(g_hotkeys.size()))
		return;
	const auto &h = g_hotkeys[static_cast<size_t>(index)];
	if (h.func)
		h.func(h.data, static_cast<obs_hotkey_id>(index), nullptr, pressed);
}

int hotkey_count()
{
	return static_cast<int>(g_hotkeys.size());
}

const std::vector<obs_key_combination_t> &hotkey_bindings(const char *name)
{
	static const std::vector<obs_key_combination_t> empty;
	if (!name)
		return empty;
	for (auto it = g_hotkeys.rbegin(); it != g_hotkeys.rend(); ++it)
		if (it->name == name)
			return it->bindings;
	return empty;
}

void set_profile_config_string(const char *section, const char *name, const char *value)
{
	if (section && name)
		g_profile_config[std::string(section) + "/" + name] = value ? value : "";
}

void fire_frontend_event(enum obs_frontend_event event)
{
	if (g_frontend_callback)
		g_frontend_callback(event, g_frontend_callback_data);
}

std::string profile_config_string(const char *section, const char *name)
{
	if (!section || !name)
		return {};
	auto it = g_profile_config.find(std::string(section) + "/" + name);
	return it == g_profile_config.end() ? std::string() : it->second;
}

bool hotkeys_active_from(int index)
{
	if (index < 0 || index >= static_cast<int>(g_hotkeys.size()))
		return false;
	for (size_t i = static_cast<size_t>(index); i < g_hotkeys.size(); ++i)
		if (!g_hotkeys[i].func || !g_hotkeys[i].active)
			return false;
	return true;
}

namespace {
std::vector<std::string> g_uniforms;
}

void note_uniform_set(const std::string &name)
{
	g_uniforms.push_back(name);
}

const std::vector<std::string> &uploaded_uniforms()
{
	return g_uniforms;
}

void clear_uniforms()
{
	g_uniforms.clear();
}

void reset_graphics_counters()
{
	g_draw_calls = 0;
	g_sprite_draw_calls = 0;
	g_render_starts = 0;
	g_vertex_calls = 0;
	g_color_calls = 0;
}

int graphics_draw_calls()
{
	return g_draw_calls;
}

int graphics_sprite_draw_calls()
{
	return g_sprite_draw_calls;
}

int graphics_render_starts()
{
	return g_render_starts;
}

int graphics_vertex_calls()
{
	return g_vertex_calls;
}

int graphics_color_calls()
{
	return g_color_calls;
}

void reset_property_update_calls()
{
	g_property_updates = 0;
}

int property_update_calls()
{
	return g_property_updates;
}

void set_fail_vertex_buffers(bool fail)
{
	g_fail_vertex_buffers = fail;
}

} // namespace fakeobs

// ---------------------------------------------------------------------------
// Minimal obs_data <-> JSON for the profiles round-trip test
// ---------------------------------------------------------------------------
namespace {

void json_escape(std::string &out, const std::string &text)
{
	out += '"';
	for (const unsigned char c : text) {
		switch (c) {
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		case '\b':
			out += "\\b";
			break;
		case '\f':
			out += "\\f";
			break;
		default:
			if (c < 0x20) {
				char buf[8];
				std::snprintf(buf, sizeof(buf), "\\u%04x", c);
				out += buf;
			} else {
				out += static_cast<char>(c);
			}
		}
	}
	out += '"';
}

void write_obs_data(std::string &out, const obs_data_t *data)
{
	out += '{';
	bool first = true;
	auto comma = [&]() {
		if (!first)
			out += ',';
		first = false;
	};
	auto key = [&](const std::string &name) {
		comma();
		json_escape(out, name);
		out += ':';
	};

	for (const auto &[name, value] : data->bools) {
		key(name);
		out += value ? "true" : "false";
	}
	for (const auto &[name, value] : data->ints) {
		key(name);
		out += std::to_string(value);
	}
	for (const auto &[name, value] : data->doubles) {
		key(name);
		char buf[64];
		std::snprintf(buf, sizeof(buf), "%.17g", value);
		out += buf;
	}
	for (const auto &[name, value] : data->strings) {
		key(name);
		json_escape(out, value);
	}
	for (const auto &[name, value] : data->objs) {
		key(name);
		if (value)
			write_obs_data(out, value);
		else
			out += "{}";
	}
	for (const auto &[name, array] : data->arrays) {
		key(name);
		out += '[';
		bool first_item = true;
		if (array) {
			for (const obs_data_t *item : array->items) {
				if (!first_item)
					out += ',';
				first_item = false;
				if (item)
					write_obs_data(out, item);
				else
					out += "{}";
			}
		}
		out += ']';
	}
	out += '}';
}

obs_data_t *obs_data_from_json_value(const topscreen::core::json::Value &value)
{
	if (!value.is_object())
		return nullptr;
	auto *data = obs_data_create();
	for (const auto &[name, child] : *value.object) {
		switch (child.type) {
		case topscreen::core::json::Value::Bool:
			data->bools[name] = child.as_bool();
			break;
		case topscreen::core::json::Value::Number: {
			/* JSON has no int/double distinction; store both so the fake
			 * get_int() and get_double() agree with the real OBS data. */
			const double n = child.as_number();
			data->doubles[name] = n;
			if (n >= -9.2e18 && n <= 9.2e18 && n == static_cast<double>(static_cast<long long>(n)))
				data->ints[name] = static_cast<long long>(n);
			break;
		}
		case topscreen::core::json::Value::String:
			data->strings[name] = child.as_string();
			break;
		case topscreen::core::json::Value::Object_:
			if (obs_data_t *nested = obs_data_from_json_value(child))
				data->objs[name] = nested;
			break;
		case topscreen::core::json::Value::Array_: {
			auto *array = new obs_data_array();
			if (child.array) {
				for (const auto &item : *child.array) {
					if (obs_data_t *entry = obs_data_from_json_value(item))
						array->items.push_back(entry);
				}
			}
			data->arrays[name] = array;
			break;
		}
		default:
			break;
		}
	}
	return data;
}

} // namespace

extern "C" {

void blog(int log_level, const char *format, ...)
{
	(void)log_level;
	va_list ap;
	va_start(ap, format);
	std::fprintf(stderr, "[fakeobs] ");
	std::vfprintf(stderr, format, ap);
	std::fprintf(stderr, "\n");
	va_end(ap);
}

void bfree(void *ptr)
{
	std::free(ptr);
}

const char *obs_module_text(const char *lookup_string)
{
	return lookup_string;
}

obs_module_t *obs_current_module(void)
{
	return nullptr;
}

char *obs_find_module_file(obs_module_t *, const char *file)
{
	if (!file)
		return nullptr;
	std::string path = g_data_dir.empty() ? std::string(file) : (g_data_dir + "/" + file);
	char *out = static_cast<char *>(std::malloc(path.size() + 1));
	if (!out)
		return nullptr;
	std::memcpy(out, path.c_str(), path.size() + 1);
	return out;
}

void obs_register_source_s(const obs_source_info *info, size_t)
{
	g_registered = info;
}

void obs_enter_graphics(void) {}
void obs_leave_graphics(void) {}

// ---- obs_data ----
obs_data_t *obs_data_create(void)
{
	return new obs_data();
}

void obs_data_addref(obs_data_t *data)
{
	if (data)
		++data->refs;
}

void obs_data_release(obs_data_t *data)
{
	if (!data || --data->refs > 0)
		return;
	for (auto &entry : data->objs)
		obs_data_release(entry.second);
	for (auto &entry : data->arrays)
		obs_data_array_release(entry.second);
	delete data;
}

bool obs_data_get_bool(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return false;
	auto it = data->bools.find(name);
	if (it != data->bools.end())
		return it->second;
	auto d = data->def_bools.find(name);
	return d != data->def_bools.end() ? d->second : false;
}

long long obs_data_get_int(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return 0;
	auto it = data->ints.find(name);
	if (it != data->ints.end())
		return it->second;
	auto d = data->def_ints.find(name);
	return d != data->def_ints.end() ? d->second : 0;
}

double obs_data_get_double(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return 0.0;
	auto it = data->doubles.find(name);
	if (it != data->doubles.end())
		return it->second;
	auto d = data->def_doubles.find(name);
	return d != data->def_doubles.end() ? d->second : 0.0;
}

const char *obs_data_get_string(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return "";
	auto it = data->strings.find(name);
	if (it != data->strings.end())
		return it->second.c_str();
	auto d = data->def_strings.find(name);
	return d != data->def_strings.end() ? d->second.c_str() : "";
}

void obs_data_set_bool(obs_data_t *data, const char *name, bool value)
{
	if (data && name)
		data->bools[name] = value;
}

void obs_data_set_int(obs_data_t *data, const char *name, long long value)
{
	if (data && name)
		data->ints[name] = value;
}

void obs_data_set_double(obs_data_t *data, const char *name, double value)
{
	if (data && name)
		data->doubles[name] = value;
}

void obs_data_set_string(obs_data_t *data, const char *name, const char *value)
{
	if (data && name)
		data->strings[name] = value ? value : "";
}

void obs_data_set_default_bool(obs_data_t *data, const char *name, bool value)
{
	if (data && name)
		data->def_bools[name] = value;
}

void obs_data_set_default_int(obs_data_t *data, const char *name, long long value)
{
	if (data && name)
		data->def_ints[name] = value;
}

void obs_data_set_default_double(obs_data_t *data, const char *name, double value)
{
	if (data && name)
		data->def_doubles[name] = value;
}

void obs_data_set_default_string(obs_data_t *data, const char *name, const char *value)
{
	if (data && name)
		data->def_strings[name] = value ? value : "";
}

bool obs_data_has_user_value(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return false;
	return data->bools.count(name) || data->ints.count(name) || data->doubles.count(name) ||
	       data->strings.count(name);
}

bool obs_data_has_default_value(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return false;
	return data->def_bools.count(name) || data->def_ints.count(name) ||
	       data->def_doubles.count(name) || data->def_strings.count(name);
}

void obs_data_set_obj(obs_data_t *data, const char *name, obs_data_t *obj)
{
	if (!data || !name)
		return;
	if (obj)
		obs_data_addref(obj);
	auto it = data->objs.find(name);
	if (it != data->objs.end() && it->second != obj)
		obs_data_release(it->second);
	data->objs[name] = obj;
}

obs_data_t *obs_data_get_obj(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return nullptr;
	auto it = data->objs.find(name);
	if (it == data->objs.end())
		return nullptr;
	obs_data_addref(it->second);
	return it->second;
}

obs_data_array_t *obs_data_get_array(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return nullptr;
	auto it = data->arrays.find(name);
	if (it == data->arrays.end())
		return nullptr;
	obs_data_array_addref(it->second);
	return it->second;
}

void obs_data_set_array(obs_data_t *data, const char *name, obs_data_array_t *array)
{
	if (!data || !name)
		return;
	if (array)
		obs_data_array_addref(array);
	auto it = data->arrays.find(name);
	if (it != data->arrays.end() && it->second != array)
		obs_data_array_release(it->second);
	data->arrays[name] = array;
}

const char *obs_data_get_json(obs_data_t *data)
{
	static thread_local std::string json;
	json.clear();
	if (data)
		write_obs_data(json, data);
	return json.c_str();
}

size_t obs_data_array_count(obs_data_array_t *array)
{
	return array ? array->items.size() : 0;
}

obs_data_t *obs_data_array_item(obs_data_array_t *array, size_t idx)
{
	if (!array || idx >= array->items.size())
		return nullptr;
	obs_data_addref(array->items[idx]);
	return array->items[idx];
}

void obs_data_array_addref(obs_data_array_t *array)
{
	if (array)
		++array->refs;
}

void obs_data_array_release(obs_data_array_t *array)
{
	if (!array || --array->refs > 0)
		return;
	for (obs_data_t *item : array->items)
		obs_data_release(item);
	delete array;
}

/* Test-only JSON support: the profiles export/import test writes and reads a
 * real bundle, so the fake behaves like the OBS API for this subset. */
obs_data_t *obs_data_create_from_json(const char *json_string)
{
	if (!json_string)
		return nullptr;
	topscreen::core::json::Value value;
	std::string error;
	if (!topscreen::core::json::parse(json_string, &value, &error))
		return nullptr;
	return obs_data_from_json_value(value);
}

bool obs_data_save_json_pretty_safe(obs_data_t *data, const char *file, const char *,
				    const char *)
{
	if (!data || !file)
		return false;
	std::string text;
	write_obs_data(text, data);
	std::ofstream out(file, std::ios::binary);
	if (!out)
		return false;
	out << text;
	return static_cast<bool>(out);
}

// ---- obs_source ----
uint64_t os_gettime_ns(void)
{
	return static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::steady_clock::now().time_since_epoch())
			.count());
}

obs_source_t *obs_frontend_get_current_scene(void)
{
	static obs_source_t scene;
	scene.name = g_current_scene_name;
	return &scene;
}

void obs_frontend_get_scenes(struct obs_frontend_source_list *)
{
}

obs_scene_t *obs_scene_from_source(const obs_source_t *)
{
	return nullptr;
}

obs_sceneitem_t *obs_scene_find_source(obs_scene_t *, const char *)
{
	return nullptr;
}

obs_sceneitem_t *obs_scene_add(obs_scene_t *, obs_source_t *)
{
	return nullptr;
}

void obs_scene_enum_items(obs_scene_t *, bool (*)(obs_scene_t *, obs_sceneitem_t *, void *), void *)
{
}

obs_source_t *obs_sceneitem_get_source(const obs_sceneitem_t *)
{
	return nullptr;
}

const char *obs_source_get_id(const obs_source_t *)
{
	return "";
}

obs_source_t *obs_source_create(const char *, const char *, obs_data_t *, obs_data_t *)
{
	return nullptr;
}

void obs_source_release(obs_source_t *) {}

void obs_source_set_audio_mixers(obs_source_t *, uint32_t) {}

obs_data_t *obs_source_get_settings(const obs_source_t *source)
{
	if (!source)
		return nullptr;
	obs_data_addref(source->settings);
	return source->settings;
}

void obs_source_update_properties(obs_source_t *)
{
	++g_property_updates;
}

void obs_source_update(obs_source_t *source, obs_data_t *settings)
{
	if (!source || !settings)
		return;
	// Our get_settings returns the object itself; copy for safety.
	*source->settings = *settings;
}

const char *obs_source_get_name(const obs_source_t *source)
{
	if (source && !source->name.empty())
		return source->name.c_str();
	return "fake-parent";
}

uint32_t obs_source_get_base_width(obs_source_t *source)
{
	return source ? source->base_w : 0;
}

uint32_t obs_source_get_base_height(obs_source_t *source)
{
	return source ? source->base_h : 0;
}

obs_properties_t *obs_source_properties(const obs_source_t *source)
{
	if (source && source->info && source->info->get_properties)
		return source->info->get_properties(nullptr);
	return obs_properties_create();
}

void obs_source_skip_video_filter(obs_source_t *) {}

bool obs_source_process_filter_begin(obs_source_t *, enum gs_color_format, enum obs_allow_direct_render)
{
	return true;
}

void obs_source_process_filter_end(obs_source_t *, gs_effect_t *, uint32_t, uint32_t) {}

void obs_source_process_filter_tech_end(obs_source_t *, gs_effect_t *, uint32_t, uint32_t, const char *) {}

obs_source_t *obs_filter_get_parent(const obs_source_t *filter)
{
	return filter ? filter->parent : nullptr;
}

obs_source_t *obs_filter_get_target(const obs_source_t *filter)
{
	return filter ? filter->parent : nullptr;
}

// ---- obs_properties ----
obs_properties_t *obs_properties_create(void)
{
	return new obs_properties();
}

void obs_properties_destroy(obs_properties_t *props)
{
	if (!props)
		return;
	for (auto &kv : props->items)
		delete kv.second;
	for (auto &kv : props->groups)
		obs_properties_destroy(kv.second);
	delete props;
}

obs_property_t *obs_properties_get(obs_properties_t *props, const char *property)
{
	if (!props || !property)
		return nullptr;
	auto it = props->items.find(property);
	if (it != props->items.end())
		return it->second;
	/* Nested groups, like the real obs_properties_get(). */
	for (const auto &g : props->groups) {
		obs_property_t *found = obs_properties_get(g.second, property);
		if (found)
			return found;
	}
	return nullptr;
}

obs_property_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_button(obs_properties_t *props, const char *name, const char *,
					  obs_property_clicked_t)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_button2(obs_properties_t *props, const char *name, const char *,
					   obs_property_clicked_t, void *)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_int(obs_properties_t *props, const char *name, const char *, int, int,
				       int)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_color(obs_properties_t *props, const char *name, const char *)
{
	return add_prop(props, name);
}

void obs_property_set_visible(obs_property_t *prop, bool visible)
{
	if (prop)
		prop->visible = visible;
}

obs_property_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *,
					enum obs_text_type)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_float_slider(obs_properties_t *props, const char *name, const char *,
						double, double, double)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_list(obs_properties_t *props, const char *name, const char *,
					enum obs_combo_type, enum obs_combo_format)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_int_slider(obs_properties_t *props, const char *name, const char *, int,
					      int, int)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_path(obs_properties_t *props, const char *name, const char *,
					enum obs_path_type, const char *, const char *)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_group(obs_properties_t *props, const char *name, const char *,
					 enum obs_group_type, obs_properties_t *group)
{
	if (props && group && name)
		props->groups[name] = group;
	return add_prop(props, name);
}

size_t obs_property_list_add_int(obs_property_t *, const char *, long long)
{
	return 0;
}

size_t obs_property_list_add_string(obs_property_t *, const char *, const char *)
{
	return 0;
}

void obs_property_set_modified_callback(obs_property_t *prop, obs_property_modified_t callback)
{
	if (prop)
		prop->modified = callback;
}

void obs_property_set_long_description(obs_property_t *, const char *) {}

// ---- config / frontend hotkeys ----
struct config_data {
	std::map<std::string, std::string> values;
};

config_t *obs_frontend_get_profile_config()
{
	static config_data config;
	return &config;
}

const char *config_get_string(config_t *, const char *section, const char *name)
{
	if (!section || !name)
		return nullptr;
	const std::string key = std::string(section) + "/" + name;
	auto it = g_profile_config.find(key);
	return it == g_profile_config.end() ? nullptr : it->second.c_str();
}

void config_set_string(config_t *, const char *section, const char *name, const char *value)
{
	if (section && name)
		g_profile_config[std::string(section) + "/" + name] = value ? value : "";
}

int64_t config_get_int(config_t *, const char *section, const char *name)
{
	const char *value = config_get_string(nullptr, section, name);
	if (!value || !*value)
		return 0;
	return std::strtoll(value, nullptr, 10);
}

void config_set_int(config_t *, const char *section, const char *name, int64_t value)
{
	const std::string text = std::to_string(value);
	config_set_string(nullptr, section, name, text.c_str());
}

bool config_remove_value(config_t *, const char *section, const char *name)
{
	if (!section || !name)
		return false;
	return g_profile_config.erase(std::string(section) + "/" + name) > 0;
}

int config_save_safe(config_t *, const char *, const char *)
{
	return 0;
}

void obs_frontend_add_event_callback(obs_frontend_event_cb callback, void *data)
{
	g_frontend_callback = callback;
	g_frontend_callback_data = data;
}

void obs_frontend_remove_event_callback(obs_frontend_event_cb callback, void *data)
{
	if (g_frontend_callback == callback && g_frontend_callback_data == data) {
		g_frontend_callback = nullptr;
		g_frontend_callback_data = nullptr;
	}
}

obs_key_t obs_key_from_name(const char *name)
{
	if (!name)
		return OBS_KEY_NONE;
	if (std::strncmp(name, "OBS_KEY_", 8) == 0)
		name += 8;
	if (name[0] == '\0' || name[1] != '\0')
		return OBS_KEY_NONE;
	if (name[0] >= 'A' && name[0] <= 'Z')
		return static_cast<obs_key_t>(OBS_KEY_A + (name[0] - 'A'));
	return OBS_KEY_NONE;
}

const char *obs_key_to_name(obs_key_t key)
{
	if (key >= OBS_KEY_A && key <= OBS_KEY_Z) {
		static const char *const names[] = {
			"OBS_KEY_A", "OBS_KEY_B", "OBS_KEY_C", "OBS_KEY_D", "OBS_KEY_E",
			"OBS_KEY_F", "OBS_KEY_G", "OBS_KEY_H", "OBS_KEY_I", "OBS_KEY_J",
			"OBS_KEY_K", "OBS_KEY_L", "OBS_KEY_M", "OBS_KEY_N", "OBS_KEY_O",
			"OBS_KEY_P", "OBS_KEY_Q", "OBS_KEY_R", "OBS_KEY_S", "OBS_KEY_T",
			"OBS_KEY_U", "OBS_KEY_V", "OBS_KEY_W", "OBS_KEY_X", "OBS_KEY_Y",
			"OBS_KEY_Z",
		};
		return names[key - OBS_KEY_A];
	}
	return "";
}

// ---- obs_hotkey ----
obs_hotkey_id register_hotkey(const char *name, obs_hotkey_func func, void *data)
{
	RegisteredHotkey h;
	h.name = name ? name : "";
	h.func = func;
	h.data = data;
	g_hotkeys.push_back(h);
	return static_cast<obs_hotkey_id>(g_hotkeys.size());
}

obs_hotkey_id obs_hotkey_register_source(obs_source_t *, const char *name, const char *,
					 obs_hotkey_func func, void *data)
{
	return register_hotkey(name, func, data);
}

obs_hotkey_id obs_hotkey_register_frontend(const char *name, const char *, obs_hotkey_func func,
					    void *data)
{
	return register_hotkey(name, func, data);
}

void obs_hotkey_load_bindings(obs_hotkey_id id, obs_key_combination_t *combinations, size_t num)
{
	const size_t index = static_cast<size_t>(id) - 1;
	if (index >= g_hotkeys.size())
		return;
	auto &bindings = g_hotkeys[index].bindings;
	bindings.clear();
	if (!combinations || num == 0)
		return;
	bindings.assign(combinations, combinations + num);
}

obs_data_array_t *obs_hotkey_save(obs_hotkey_id id)
{
	const size_t index = static_cast<size_t>(id) - 1;
	auto *array = new obs_data_array();
	if (index >= g_hotkeys.size())
		return array;
	for (const auto &combo : g_hotkeys[index].bindings) {
		obs_data_t *item = obs_data_create();
		obs_data_set_string(item, "key", obs_key_to_name(combo.key));
		if (combo.modifiers & INTERACT_CONTROL_KEY)
			obs_data_set_bool(item, "control", true);
		if (combo.modifiers & INTERACT_SHIFT_KEY)
			obs_data_set_bool(item, "shift", true);
		if (combo.modifiers & INTERACT_ALT_KEY)
			obs_data_set_bool(item, "alt", true);
		array->items.push_back(item);
	}
	return array;
}

void obs_hotkey_load(obs_hotkey_id id, obs_data_array_t *data)
{
	const size_t index = static_cast<size_t>(id) - 1;
	if (index >= g_hotkeys.size())
		return;
	auto &bindings = g_hotkeys[index].bindings;
	bindings.clear();
	const size_t count = obs_data_array_count(data);
	for (size_t i = 0; i < count; ++i) {
		obs_data_t *item = obs_data_array_item(data, i);
		if (!item)
			continue;
		obs_key_combination_t combo = {};
		combo.key = obs_key_from_name(obs_data_get_string(item, "key"));
		if (obs_data_get_bool(item, "control"))
			combo.modifiers |= INTERACT_CONTROL_KEY;
		if (obs_data_get_bool(item, "shift"))
			combo.modifiers |= INTERACT_SHIFT_KEY;
		if (obs_data_get_bool(item, "alt"))
			combo.modifiers |= INTERACT_ALT_KEY;
		bindings.push_back(combo);
		obs_data_release(item);
	}
}

void obs_hotkey_unregister(obs_hotkey_id id)
{
	const size_t index = static_cast<size_t>(id) - 1;
	if (index < g_hotkeys.size())
		g_hotkeys[index].active = false;
}

// ---- graphics ----
gs_effect_t *gs_effect_create_from_file(const char *file, char **error)
{
	if (error)
		*error = nullptr;
	if (!file)
		return nullptr;
	// Validates that the file exists (exercises data path resolution).
	std::ifstream in(file);
	if (!in)
		return nullptr;
	return reinterpret_cast<gs_effect_t *>(&g_effect_dummy);
}

void gs_effect_destroy(gs_effect_t *) {}

struct obs_param {
	std::string name;
};

gs_eparam_t *gs_effect_get_param_by_name(const gs_effect_t *, const char *name)
{
	/* Stable per-name pointers so the harness can tell which uniforms the
	 * render path actually uploads (D3D11 aborts the draw otherwise). */
	static std::vector<std::unique_ptr<obs_param>> params;
	for (const auto &p : params)
		if (p->name == (name ? name : ""))
			return reinterpret_cast<gs_eparam_t *>(p.get());
	params.push_back(std::make_unique<obs_param>(obs_param{name ? name : ""}));
	return reinterpret_cast<gs_eparam_t *>(params.back().get());
}

namespace {
void note_param_set(gs_eparam_t *param)
{
	if (param)
		fakeobs::note_uniform_set(reinterpret_cast<obs_param *>(param)->name);
}

const char *param_name(gs_eparam_t *param)
{
	return param ? reinterpret_cast<obs_param *>(param)->name.c_str() : "";
}
} // namespace

void gs_effect_set_float(gs_eparam_t *p, float) { note_param_set(p); }
void gs_effect_set_color(gs_eparam_t *p, uint32_t) { note_param_set(p); }
void gs_effect_set_vec2(gs_eparam_t *p, const struct vec2 *) { note_param_set(p); }
void gs_effect_set_val(gs_eparam_t *p, const void *, size_t) { note_param_set(p); }
void gs_effect_set_texture(gs_eparam_t *p, gs_texture_t *) { note_param_set(p); }
void gs_effect_set_vec4(gs_eparam_t *p, const struct vec4 *) { note_param_set(p); }
void gs_effect_set_matrix4(gs_eparam_t *p, const struct matrix4 *) { note_param_set(p); }

void gs_texture_set_image(gs_texture_t *, const uint8_t *, uint32_t, bool) {}

const char *fakeobs_uniform_name(gs_eparam_t *param)
{
	return param_name(param);
}

gs_technique_t *gs_effect_get_technique(const gs_effect_t *, const char *)
{
	return reinterpret_cast<gs_technique_t *>(&g_technique_dummy);
}

size_t gs_technique_begin(gs_technique_t *)
{
	return 1;
}

bool gs_technique_begin_pass(gs_technique_t *, size_t)
{
	return true;
}

void gs_technique_end_pass(gs_technique_t *) {}
void gs_technique_end(gs_technique_t *) {}

void gs_matrix_push(void) {}
void gs_matrix_pop(void) {}
void gs_matrix_translate3f(float, float, float) {}
void gs_matrix_scale3f(float, float, float) {}

void gs_draw_sprite_subregion(gs_texture_t *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
	++g_sprite_draw_calls;
}

void gs_render_start(bool) { ++g_render_starts; }
void gs_vertex2f(float, float) { ++g_vertex_calls; }
void gs_texcoord(float, float, int) {}
void gs_color(uint32_t) { ++g_color_calls; }
/* Deliberately a no-op, matching libobs 30.x: this is the regression the
 * text-batching test guards against. */
void gs_color4v(const struct vec4 *) {}

gs_vertbuffer_t *gs_render_save()
{
	if (g_fail_vertex_buffers)
		return nullptr;
	return reinterpret_cast<gs_vertbuffer_t *>(&g_vertex_dummy);
}

void gs_vertexbuffer_destroy(gs_vertbuffer_t *) {}
void gs_load_vertexbuffer(gs_vertbuffer_t *) {}
void gs_load_indexbuffer(gs_indexbuffer_t *) {}

void gs_draw(enum gs_draw_mode, uint32_t, uint32_t) { ++g_draw_calls; }

void gs_blend_state_push(void) {}
void gs_blend_state_pop(void) {}
void gs_blend_function(enum gs_blend_type, enum gs_blend_type) {}

gs_texture_t *gs_texture_create(uint32_t, uint32_t, enum gs_color_format, uint32_t, const uint8_t **, uint32_t)
{
	return reinterpret_cast<gs_texture_t *>(&g_texture_dummy);
}

void gs_texture_destroy(gs_texture_t *) {}

} // extern "C"

namespace fakeobs {

bool property_visible(obs_property_t *prop)
{
	return prop && prop->visible;
}

bool invoke_property_modified(obs_properties_t *props, obs_property_t *prop,
			      obs_data_t *settings)
{
	return prop && prop->modified ? prop->modified(props, prop, settings) : false;
}

} // namespace fakeobs
