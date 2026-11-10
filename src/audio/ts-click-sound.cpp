#include "ts-click-sound.hpp"

#include "ts-click-audio.hpp"

#include <obs-module.h>

#include <algorithm>
#include <vector>

#include "core/file.hpp"

namespace topscreen {

bool ClickSound::load_slot(Slot *slot, const char *module_file)
{
	if (!module_file || !*module_file) {
		*slot = Slot{};
		return false;
	}
	/* The resolved path is absolute; compare the module-relative name. */
	if (slot->loaded && slot->module == module_file)
		return true;

	char *file_path = obs_module_file(module_file);
	if (!file_path) {
		blog(LOG_WARNING, "[TopScreen] sound not found: %s", module_file);
		*slot = Slot{};
		return false;
	}

	std::vector<uint8_t> bytes;
	const bool ok = core::read_file_bytes(file_path, &bytes);
	if (!ok) {
		blog(LOG_WARNING, "[TopScreen] sound unreadable: %s", module_file);
		bfree(file_path);
		*slot = Slot{};
		return false;
	}

	std::string err;
	core::WavData wav;
	if (!core::wav_parse(bytes.data(), bytes.size(), &wav, &err)) {
		blog(LOG_WARNING, "[TopScreen] invalid WAV (%s): %s", module_file, err.c_str());
		bfree(file_path);
		*slot = Slot{};
		return false;
	}

	slot->module = module_file;
	slot->path = file_path;
	bfree(file_path);
	slot->wav = std::make_shared<const core::WavData>(std::move(wav));
	slot->loaded = true;
	blog(LOG_INFO, "[TopScreen] sound loaded: %s", module_file);
	return true;
}

bool ClickSound::load(const char *left_module_file, const char *right_module_file)
{
	std::lock_guard<std::mutex> lock(mutex_);
	const bool l = load_slot(&left_, left_module_file);
	const bool r = load_slot(&right_, right_module_file);
	loaded_ = l || r;
	return loaded_;
}

core::WavData ClickSound::wav() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return left_.wav ? *left_.wav : core::WavData{};
}

std::string ClickSound::loaded_path(bool right) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return right ? right_.path : left_.path;
}

void ClickSound::release()
{
	std::lock_guard<std::mutex> lock(mutex_);
	loaded_ = false;
	left_ = Slot{};
	right_ = Slot{};
}

void ClickSound::play(bool right, double volume)
{
	std::shared_ptr<const core::WavData> wav;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		const Slot &slot = right ? right_ : left_;
		if (!slot.loaded || slot.path.empty())
			return;
		wav = slot.wav;
	}

	if (wav)
		ClickAudioBus::instance().push(std::move(wav), std::clamp(volume, 0.0, 1.0));
}

} // namespace topscreen
