#include "fake_libobs.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

// ---------------------------------------------------------------------------
// Estado interno
// ---------------------------------------------------------------------------
namespace {

std::string g_data_dir;
const obs_source_info *g_registered = nullptr;

struct RegisteredHotkey {
	std::string name;
	obs_hotkey_func func = nullptr;
	void *data = nullptr;
	bool active = true;
};
std::vector<RegisteredHotkey> g_hotkeys;

char g_effect_dummy = 0;
char g_param_dummy = 0;
char g_technique_dummy = 0;
char g_texture_dummy = 0;

obs_property_t *add_prop(obs_properties_t *props, const char *name)
{
	auto *p = new obs_property();
	p->name = name ? name : "";
	props->items[p->name] = p;
	return p;
}

} // namespace

namespace fakeobs {

void set_data_dir(const std::string &dir)
{
	g_data_dir = dir;
}

obs_source_t *make_source(const obs_source_info *info, obs_source_t *parent, uint32_t w, uint32_t h)
{
	auto *s = new obs_source();
	s->info = info;
	s->parent = parent;
	s->base_w = w;
	s->base_h = h;
	s->settings = obs_data_create();
	return s;
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

} // namespace fakeobs

// ---------------------------------------------------------------------------
// Implementacao da API do OBS
// ---------------------------------------------------------------------------

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

void obs_data_release(obs_data_t *) {}

bool obs_data_get_bool(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return false;
	auto it = data->bools.find(name);
	return it != data->bools.end() ? it->second : false;
}

long long obs_data_get_int(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return 0;
	auto it = data->ints.find(name);
	return it != data->ints.end() ? it->second : 0;
}

double obs_data_get_double(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return 0.0;
	auto it = data->doubles.find(name);
	return it != data->doubles.end() ? it->second : 0.0;
}

const char *obs_data_get_string(obs_data_t *data, const char *name)
{
	if (!data || !name)
		return "";
	auto it = data->strings.find(name);
	return it != data->strings.end() ? it->second.c_str() : "";
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
	if (data && name && data->bools.find(name) == data->bools.end())
		data->bools[name] = value;
}

void obs_data_set_default_int(obs_data_t *data, const char *name, long long value)
{
	if (data && name && data->ints.find(name) == data->ints.end())
		data->ints[name] = value;
}

void obs_data_set_default_double(obs_data_t *data, const char *name, double value)
{
	if (data && name && data->doubles.find(name) == data->doubles.end())
		data->doubles[name] = value;
}

void obs_data_set_default_string(obs_data_t *data, const char *name, const char *value)
{
	if (data && name && data->strings.find(name) == data->strings.end())
		data->strings[name] = value ? value : "";
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
	/* no stub, defaults sao gravados no mesmo mapa de valores */
	return obs_data_has_user_value(data, name);
}

// ---- obs_source ----
obs_data_t *obs_source_get_settings(const obs_source_t *source)
{
	return source ? source->settings : nullptr;
}

void obs_source_update(obs_source_t *source, obs_data_t *settings)
{
	if (!source || !settings)
		return;
	// Nosso get_settings devolve o proprio objeto; copia por seguranca.
	*source->settings = *settings;
}

const char *obs_source_get_name(const obs_source_t *source)
{
	(void)source;
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
	delete props;
}

obs_property_t *obs_properties_get(obs_properties_t *props, const char *property)
{
	if (!props || !property)
		return nullptr;
	auto it = props->items.find(property);
	return it != props->items.end() ? it->second : nullptr;
}

obs_property_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *)
{
	return add_prop(props, name);
}

obs_property_t *obs_properties_add_color(obs_properties_t *props, const char *name, const char *)
{
	return add_prop(props, name);
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
					 enum obs_group_type, obs_properties_t *)
{
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

// ---- obs_hotkey ----
obs_hotkey_id obs_hotkey_register_source(obs_source_t *, const char *name, const char *,
					 obs_hotkey_func func, void *data)
{
	RegisteredHotkey h;
	h.name = name ? name : "";
	h.func = func;
	h.data = data;
	g_hotkeys.push_back(h);
	return static_cast<obs_hotkey_id>(g_hotkeys.size());
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
	// Valida que o arquivo existe (exercita a resolucao de caminho de dados).
	std::ifstream in(file);
	if (!in)
		return nullptr;
	return reinterpret_cast<gs_effect_t *>(&g_effect_dummy);
}

void gs_effect_destroy(gs_effect_t *) {}

gs_eparam_t *gs_effect_get_param_by_name(const gs_effect_t *, const char *)
{
	return reinterpret_cast<gs_eparam_t *>(&g_param_dummy);
}

void gs_effect_set_float(gs_eparam_t *, float) {}
void gs_effect_set_color(gs_eparam_t *, uint32_t) {}
void gs_effect_set_vec2(gs_eparam_t *, const struct vec2 *) {}
void gs_effect_set_val(gs_eparam_t *, const void *, size_t) {}
void gs_effect_set_texture(gs_eparam_t *, gs_texture_t *) {}
void gs_effect_set_vec4(gs_eparam_t *, const struct vec4 *) {}

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

void gs_draw_sprite_subregion(gs_texture_t *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {}

void gs_blend_state_push(void) {}
void gs_blend_state_pop(void) {}
void gs_blend_function(enum gs_blend_type, enum gs_blend_type) {}

gs_texture_t *gs_texture_create(uint32_t, uint32_t, enum gs_color_format, uint32_t, const uint8_t **, uint32_t)
{
	return reinterpret_cast<gs_texture_t *>(&g_texture_dummy);
}

void gs_texture_destroy(gs_texture_t *) {}

} // extern "C"
