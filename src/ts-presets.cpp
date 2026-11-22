#include "ts-presets.hpp"

#include <obs-module.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>
#include <vector>

#include "core/file.hpp"
#include "ts-settings.hpp"

namespace topscreen {

namespace {

const char *const kPresetFiles[] = {
	"presets/demo.json",
	"presets/tutorial.json",
	"presets/presentation.json",
	"presets/glass.json",
};

bool read_module_file(const char *name, std::string *out)
{
	char *path = obs_module_file(name);
	if (!path)
		return false;

	const bool ok = core::read_file_text(path, out);
	bfree(path);
	return ok;
}

std::filesystem::path fs_path(const std::string &utf8)
{
#if defined(_WIN32)
	const char8_t *data = reinterpret_cast<const char8_t *>(utf8.data());
	return std::filesystem::path(std::u8string(data, utf8.size()));
#else
	return std::filesystem::path(utf8);
#endif
}

std::string path_utf8(const std::filesystem::path &path)
{
	const std::u8string u8 = path.u8string();
	return std::string(reinterpret_cast<const char *>(u8.data()), u8.size());
}

std::string safe_file_name(const std::string &name)
{
	std::string out;
	out.reserve(name.size());
	for (unsigned char ch : name) {
		if (std::isalnum(ch) || ch == '-' || ch == '_')
			out.push_back(static_cast<char>(ch));
		else if (ch == ' ')
			out.push_back('_');
		else if ((ch & 0x80u) != 0)
			out.push_back('_');
	}
	if (out.empty())
		out = "look";
	return out;
}

} // namespace

std::string PresetStore::resolve_user_dir()
{
	if (!user_dir_.empty())
		return user_dir_;

	char *dir = obs_module_config_path("looks");
	if (dir) {
		user_dir_ = dir;
		bfree(dir);
	}
	return user_dir_;
}

int PresetStore::load()
{
	entries_.clear();
	current_ = -1;

	for (const char *file : kPresetFiles) {
		std::string text;
		if (!read_module_file(file, &text)) {
			blog(LOG_WARNING, "[TopScreen] preset not found: %s", file);
			continue;
		}

		core::Preset p;
		std::string err;
		if (!core::Preset::from_json(text, &p, &err)) {
			blog(LOG_WARNING, "[TopScreen] invalid preset %s: %s", file, err.c_str());
			continue;
		}
		entries_.push_back({std::move(p), std::string(), false});
	}

	const std::string dir = resolve_user_dir();
	if (!dir.empty()) {
		std::error_code ec;
		std::filesystem::create_directories(fs_path(dir), ec);
		std::vector<Entry> user;
		for (std::filesystem::directory_iterator it(fs_path(dir), ec), end;
		     !ec && it != end; it.increment(ec)) {
			if (!it->is_regular_file(ec) || it->path().extension() != ".json")
				continue;
			const std::string path = path_utf8(it->path());
			std::string text;
			if (!core::read_file_text(path.c_str(), &text))
				continue;
			core::Preset p;
			std::string err;
			if (!core::Preset::from_json(text, &p, &err)) {
				blog(LOG_WARNING, "[TopScreen] invalid user look %s: %s",
				     path.c_str(), err.c_str());
				continue;
			}
			user.push_back({std::move(p), path, true});
		}
		std::sort(user.begin(), user.end(), [](const Entry &a, const Entry &b) {
			return a.path < b.path;
		});
		for (Entry &e : user)
			entries_.push_back(std::move(e));
	}

	return count();
}

const std::string &PresetStore::name(int index) const
{
	static const std::string empty;
	if (index < 0 || index >= count())
		return empty;
	return entries_[index].preset.name;
}

bool PresetStore::is_user(int index) const
{
	return index >= 0 && index < count() && entries_[index].user;
}

bool PresetStore::apply(int index, core::Config *cfg)
{
	if (index < 0 || index >= count())
		return false;

	std::string err;
	if (!entries_[index].preset.apply(cfg, &err)) {
		blog(LOG_WARNING, "[TopScreen] failed to apply preset: %s", err.c_str());
		return false;
	}

	current_ = index;
	return true;
}

int PresetStore::next()
{
	if (entries_.empty())
		return -1;
	current_ = (current_ + 1) % count();
	return current_;
}

int PresetStore::prev()
{
	if (entries_.empty())
		return -1;
	current_ = (current_ - 1 + count()) % count();
	return current_;
}

int PresetStore::save_user(const std::string &name, const core::Config &cfg,
			   std::string *error)
{
	const std::string dir = resolve_user_dir();
	if (dir.empty()) {
		if (error)
			*error = "the plugin config directory is not available";
		return -1;
	}

	std::error_code ec;
	std::filesystem::create_directories(fs_path(dir), ec);
	if (ec) {
		if (error)
			*error = "could not create the looks directory";
		return -1;
	}

	const std::string base = safe_file_name(name);
	std::string path = dir + "/" + base + ".json";
	for (int n = 2; std::filesystem::exists(fs_path(path), ec); ++n)
		path = dir + "/" + base + "-" + std::to_string(n) + ".json";

	obs_data_t *root = obs_data_create();
	obs_data_set_string(root, "name", name.c_str());
	obs_data_set_string(root, "description", "User look");
	obs_data_t *settings = obs_data_create();
	config_store(settings, cfg);
	obs_data_set_obj(root, "settings", settings);
	obs_data_release(settings);

	const bool ok = obs_data_save_json_safe(root, path.c_str(), "tmp", "bak");
	obs_data_release(root);
	if (!ok) {
		if (error)
			*error = "could not write the look file";
		return -1;
	}

	load();
	for (int i = 0; i < count(); ++i) {
		if (entries_[i].path == path) {
			current_ = i;
			return i;
		}
	}
	if (error)
		*error = "the new look could not be reloaded";
	return -1;
}

bool PresetStore::delete_user(int index, std::string *error)
{
	if (!is_user(index)) {
		if (error)
			*error = "only user looks can be deleted";
		return false;
	}

	std::error_code ec;
	std::filesystem::remove(fs_path(entries_[index].path), ec);
	if (ec) {
		if (error)
			*error = "could not remove the look file";
		return false;
	}

	entries_.erase(entries_.begin() + index);
	if (current_ == index)
		current_ = -1;
	else if (current_ > index)
		--current_;
	return true;
}

} // namespace topscreen
