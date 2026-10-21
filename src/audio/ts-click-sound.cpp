#include "ts-click-sound.hpp"

#include <obs-module.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "core/file.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#if !defined(_WIN32)
extern char **environ;
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
	if (!module_file || !*module_file) {
		*slot = Slot{};
		return false;
	}
	/* The resolved path is absolute; compare the module-relative name. */
	if (slot->loaded && slot->module == module_file)
		return true;

#if !defined(_WIN32)
	/* Flush any queued audio from the previous sound/profile before loading a
	 * different WAV. The persistent player restarts lazily on the next click. */
	stop_persistent();
#endif

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

	slot->wav = std::move(wav);
	slot->loaded = true;
	blog(LOG_INFO, "[TopScreen] sound loaded: %s", module_file);
	return true;
}

bool ClickSound::load(const char *left_module_file, const char *right_module_file)
{
	std::lock_guard<std::mutex> lock(mutex_);
	reap_children();
	const bool l = load_slot(&left_, left_module_file);
	const bool r = load_slot(&right_, right_module_file);
	loaded_ = l || r;

#if !defined(_WIN32)
	/* Resolve the player once per filter; scanning PATH on every property
	 * change (and re-reading unchanged WAVs) would be pure overhead. */
	if (!player_checked_) {
		player_ = find_player();
		player_checked_ = true;
		persistent_checked_ = true;
		persistent_available_ =
			player_.size() >= 6 && player_.compare(player_.size() - 6, 6, "paplay") == 0;
		if (player_.empty())
			blog(LOG_WARNING, "[TopScreen] no audio player found for the click sound");
		else if (persistent_available_)
			blog(LOG_INFO, "[TopScreen] using a persistent paplay stream");
	}
#endif

	return loaded_;
}

std::string ClickSound::loaded_path(bool right) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return right ? right_.path : left_.path;
}

void ClickSound::release()
{
	std::lock_guard<std::mutex> lock(mutex_);
#if !defined(_WIN32)
	stop_persistent();
	reap_children();
	persistent_checked_ = false;
	persistent_available_ = false;
#endif
	loaded_ = false;
	left_ = Slot{};
	right_ = Slot{};
	player_.clear();
	player_checked_ = false;
	children_.clear();
}

void ClickSound::reap_children()
{
#if !defined(_WIN32)
	for (auto it = children_.begin(); it != children_.end();) {
		const pid_t pid = static_cast<pid_t>(*it);
		const pid_t r = ::waitpid(pid, nullptr, WNOHANG);
		if (r == pid || r < 0)
			it = children_.erase(it);
		else
			++it;
	}
#endif
}

#if !defined(_WIN32)
bool ClickSound::start_persistent(double volume)
{
	if (player_.empty())
		return false;

	int fds[2];
	if (::pipe(fds) != 0)
		return false;
	::fcntl(fds[1], F_SETFL, ::fcntl(fds[1], F_GETFL, 0) | O_NONBLOCK);

	posix_spawn_file_actions_t actions;
	posix_spawn_file_actions_init(&actions);
	posix_spawn_file_actions_adddup2(&actions, fds[0], STDIN_FILENO);
	posix_spawn_file_actions_addclose(&actions, fds[0]);
	posix_spawn_file_actions_addclose(&actions, fds[1]);

	char vol[32];
	std::snprintf(vol, sizeof(vol), "--volume=%d", static_cast<int>(volume * 65536.0));
	const char *argv[] = {player_.c_str(),
			      "--raw",
			      "--format=s16le",
			      "--rate=44100",
			      "--channels=1",
			      "--latency-msec=20",
			      "--client-name=TopScreen",
			      vol,
			      nullptr};

	posix_spawnattr_t attr;
	posix_spawnattr_init(&attr);
#if defined(POSIX_SPAWN_SETSID)
	posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);
#endif

	pid_t pid = 0;
	const int rc = ::posix_spawn(&pid, player_.c_str(), &actions, &attr,
				     const_cast<char *const *>(argv), environ);
	posix_spawnattr_destroy(&attr);
	posix_spawn_file_actions_destroy(&actions);
	::close(fds[0]);
	if (rc != 0) {
		::close(fds[1]);
		blog(LOG_WARNING, "[TopScreen] could not start the persistent audio player (%d)", rc);
		return false;
	}

	player_pid_ = static_cast<int>(pid);
	player_stdin_ = fds[1];
	return true;
}

bool ClickSound::ensure_persistent(double volume)
{
	(void)volume; // the PCM is scaled per click, so volume changes do not restart
	if (!persistent_checked_)
		return false;
	if (!persistent_available_)
		return false;
	if (player_stdin_ >= 0)
		return true;
	stop_persistent();
	return start_persistent(1.0);
}

void ClickSound::stop_persistent()
{
	if (player_stdin_ >= 0) {
		::close(player_stdin_);
		player_stdin_ = -1;
	}
	if (player_pid_ > 0) {
		/* Do not block the video thread waiting for paplay. Closing stdin is
		 * normally enough; the child is reaped lazily on the next click. */
		::kill(static_cast<pid_t>(player_pid_), SIGTERM);
		children_.push_back(player_pid_);
		player_pid_ = -1;
	}
}

bool ClickSound::write_pcm(const Slot &slot, double volume)
{
	if (player_stdin_ < 0 || slot.wav.samples.empty())
		return false;

	pcm_scratch_.resize(slot.wav.samples.size());
	for (size_t i = 0; i < slot.wav.samples.size(); ++i) {
		const float v = std::max(-1.0f, std::min(1.0f, slot.wav.samples[i]));
		pcm_scratch_[i] = static_cast<int16_t>(std::lround(v * volume * 32767.0f));
	}

	const uint8_t *data = reinterpret_cast<const uint8_t *>(pcm_scratch_.data());
	size_t left = pcm_scratch_.size() * sizeof(int16_t);
	while (left > 0) {
		const ssize_t n = ::write(player_stdin_, data, left);
		if (n > 0) {
			data += n;
			left -= static_cast<size_t>(n);
			continue;
		}
		if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
			break; // drop the remainder instead of blocking the video thread
		return false;
	}
	return true;
}

void ClickSound::spawn_and_play(const Slot &slot, double volume)
{
	/* Fallback for systems without paplay: spawn a one-shot player. */
	reap_children();
	if (player_.empty())
		return;

	char vol[16];
	std::vector<char *> argv;
	argv.push_back(const_cast<char *>(player_.c_str()));
	if (player_.size() >= 6 && player_.compare(player_.size() - 6, 6, "paplay") == 0) {
		std::snprintf(vol, sizeof(vol), "%d", static_cast<int>(volume * 65536.0));
		argv.push_back(const_cast<char *>("--volume"));
		argv.push_back(vol);
	}
	argv.push_back(const_cast<char *>(slot.path.c_str()));
	argv.push_back(nullptr);

	posix_spawnattr_t attr;
	posix_spawnattr_init(&attr);
#if defined(POSIX_SPAWN_SETSID)
	posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);
#endif

	pid_t pid = 0;
	const int rc = ::posix_spawn(&pid, player_.c_str(), nullptr, &attr, argv.data(), environ);
	posix_spawnattr_destroy(&attr);
	if (rc == 0)
		children_.push_back(static_cast<int>(pid));
	else
		blog(LOG_WARNING, "[TopScreen] could not start the audio player (%d)", rc);
}
#endif

void ClickSound::play(bool right, double volume)
{
	std::lock_guard<std::mutex> lock(mutex_);
	reap_children();
	const Slot &slot = right ? right_ : left_;
	if (!slot.loaded || slot.path.empty())
		return;

	if (volume < 0.0)
		volume = 0.0;
	if (volume > 1.0)
		volume = 1.0;

#if defined(_WIN32)
	/* UTF-8 -> UTF-16: OBS installation paths may contain accents. */
	const int need = MultiByteToWideChar(CP_UTF8, 0, slot.path.c_str(), -1, nullptr, 0);
	if (need > 0) {
		std::wstring wide(static_cast<size_t>(need), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, slot.path.c_str(), -1, wide.data(), need);
		PlaySoundW(wide.c_str(), nullptr, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
	}
#else
	if (ensure_persistent(volume) && write_pcm(slot, volume))
		return;

	/* A broken/dead stream falls back to the old one-shot path. */
	stop_persistent();
	spawn_and_play(slot, volume);
#endif
}

} // namespace topscreen
