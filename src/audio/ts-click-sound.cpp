#include "ts-click-sound.hpp"

#include <obs-module.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "core/file.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#else
#include <csignal>
#include <unistd.h>
#endif

namespace topscreen {

namespace {

#if !defined(_WIN32)
/** Looks for an audio player in PATH (paplay/pw-play/aplay). */
std::string find_player()
{
	const char *candidates[] = {"paplay", "pw-play", "aplay"};
	const char *env = std::getenv("PATH");
	if (!env)
		return {};

	const std::string paths(env);
	size_t start = 0;
	while (start <= paths.size()) {
		const size_t end = paths.find(':', start);
		const std::string dir = paths.substr(start, end == std::string::npos ? std::string::npos : end - start);
		if (!dir.empty()) {
			for (const char *cand : candidates) {
				const std::string full = dir + "/" + cand;
				if (::access(full.c_str(), X_OK) == 0)
					return full;
			}
		}
		if (end == std::string::npos)
			break;
		start = end + 1;
	}
	return {};
}
#endif

} // namespace

bool ClickSound::load_slot(Slot *slot, const char *module_file)
{
	if (!module_file || !*module_file)
		return false;
	if (slot->loaded && slot->path == module_file)
		return true;

	char *file_path = obs_module_file(module_file);
	if (!file_path) {
		blog(LOG_WARNING, "[TopScreen] sound not found: %s", module_file);
		return false;
	}

	std::vector<uint8_t> bytes;
	const bool ok = core::read_file_bytes(file_path, &bytes);
	if (!ok) {
		bfree(file_path);
		return false;
	}

	std::string err;
	core::WavData wav;
	if (!core::wav_parse(bytes.data(), bytes.size(), &wav, &err)) {
		blog(LOG_WARNING, "[TopScreen] invalid WAV (%s): %s", module_file, err.c_str());
		bfree(file_path);
		return false;
	}

	slot->path = file_path;
	bfree(file_path);

	slot->wav = std::move(wav);
	slot->loaded = true;
	return true;
}

bool ClickSound::load(const char *left_module_file, const char *right_module_file)
{
	const bool l = load_slot(&left_, left_module_file);
	const bool r = load_slot(&right_, right_module_file);
	loaded_ = l || r;

#if !defined(_WIN32)
	/* Auto-reap player processes: play() never blocks on waitpid. */
	static bool sigchld_ignored = false;
	if (!sigchld_ignored) {
		::signal(SIGCHLD, SIG_IGN);
		sigchld_ignored = true;
	}
	player_ = find_player();
	if (player_.empty())
		blog(LOG_WARNING, "[TopScreen] no audio player found for the click sound");
#endif

	if (loaded_)
		blog(LOG_INFO, "[TopScreen] sound loaded: L=%s R=%s", left_module_file ? left_module_file : "-",
		     right_module_file ? right_module_file : "-");
	return loaded_;
}

void ClickSound::release()
{
	loaded_ = false;
	left_ = Slot{};
	right_ = Slot{};
	player_.clear();
}

void ClickSound::play(bool right, double volume)
{
	const Slot &slot = right ? right_ : left_;
	if (!slot.loaded || slot.path.empty())
		return;

	if (volume < 0.0)
		volume = 0.0;
	if (volume > 1.0)
		volume = 1.0;
	(void)volume; // only the paplay branch (Linux) honors it

#if defined(_WIN32)
	PlaySoundA(slot.path.c_str(), nullptr, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
#else
	if (player_.empty())
		return;

	/* Detached spawn: SIGCHLD is ignored (see load()), so this never
	 * blocks the caller waiting for the player. */
	const pid_t pid = ::fork();
	if (pid != 0)
		return;

	::setsid();
	/* Only paplay honors a volume flag; the other players play as-is. */
	if (player_.size() >= 6 && player_.compare(player_.size() - 6, 6, "paplay") == 0) {
		char vol[16];
		std::snprintf(vol, sizeof(vol), "%d", static_cast<int>(volume * 65536.0));
		::execl(player_.c_str(), player_.c_str(), "--volume", vol, slot.path.c_str(),
			static_cast<char *>(nullptr));
	} else {
		::execl(player_.c_str(), player_.c_str(), slot.path.c_str(),
			static_cast<char *>(nullptr));
	}
	::_exit(127);
#endif
}

} // namespace topscreen
