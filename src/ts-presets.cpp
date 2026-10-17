#include "ts-presets.hpp"

#include <obs-module.h>

#include "core/file.hpp"

namespace topscreen {

namespace {

const char *const kPresetFiles[] = {
	"presets/demo.json",
	"presets/tutorial.json",
	"presets/apresentacao.json",
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

} // namespace

int PresetStore::load()
{
	presets_.clear();

	for (const char *file : kPresetFiles) {
		std::string text;
		if (!read_module_file(file, &text)) {
			blog(LOG_WARNING, "[TopScreen] preset nao encontrado: %s", file);
			continue;
		}

		core::Preset p;
		std::string err;
		if (!core::Preset::from_json(text, &p, &err)) {
			blog(LOG_WARNING, "[TopScreen] preset invalido %s: %s", file, err.c_str());
			continue;
		}
		presets_.push_back(std::move(p));
	}

	return static_cast<int>(presets_.size());
}

const std::string &PresetStore::name(int index) const
{
	static const std::string empty;
	if (index < 0 || index >= count())
		return empty;
	return presets_[index].name;
}

bool PresetStore::apply(int index, core::Config *cfg)
{
	if (index < 0 || index >= count())
		return false;

	std::string err;
	if (!presets_[index].apply(cfg, &err)) {
		blog(LOG_WARNING, "[TopScreen] falha ao aplicar preset: %s", err.c_str());
		return false;
	}

	current_ = index;
	return true;
}

int PresetStore::next()
{
	if (presets_.empty())
		return -1;
	current_ = (current_ + 1) % count();
	return current_;
}

int PresetStore::prev()
{
	if (presets_.empty())
		return -1;
	current_ = (current_ - 1 + count()) % count();
	return current_;
}

} // namespace topscreen
