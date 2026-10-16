#include "ts-click-sound.hpp"

#include <obs-module.h>

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace topscreen {

namespace {

#if !defined(_WIN32)
/** Procura um player de audio no PATH (paplay/pw-play/aplay). */
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

bool ClickSound::load(const char *module_file)
{
	release();

	if (!module_file || !*module_file)
		return false;

	char *file_path = obs_module_file(module_file);
	if (!file_path) {
		blog(LOG_WARNING, "[TopScreen] som nao encontrado: %s", module_file);
		return false;
	}

	std::ifstream in(file_path, std::ios::binary);
	if (!in) {
		bfree(file_path);
		return false;
	}

	const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
					 std::istreambuf_iterator<char>());

	std::string err;
	core::WavData wav;
	if (!core::wav_parse(bytes.data(), bytes.size(), &wav, &err)) {
		blog(LOG_WARNING, "[TopScreen] WAV invalido (%s): %s", module_file, err.c_str());
		bfree(file_path);
		return false;
	}

	path_ = file_path;
	bfree(file_path);

	wav_ = std::move(wav);
	loaded_ = true;

#if !defined(_WIN32)
	player_ = find_player();
	if (player_.empty())
		blog(LOG_WARNING, "[TopScreen] nenhum player de audio encontrado para o som de clique");
#endif

	blog(LOG_INFO, "[TopScreen] som carregado: %s (%d Hz, %d canais)", module_file, wav_.sample_rate,
	     wav_.channels);
	return true;
}

void ClickSound::release()
{
	loaded_ = false;
	path_.clear();
	player_.clear();
	wav_ = core::WavData{};
}

void ClickSound::play()
{
	if (!loaded_ || path_.empty())
		return;

#if defined(_WIN32)
	PlaySoundA(path_.c_str(), nullptr, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
#else
	if (player_.empty())
		return;

	/* Double-fork para nao bloquear nem deixar zumbis. */
	const pid_t pid = ::fork();
	if (pid == 0) {
		::setsid();
		const pid_t pid2 = ::fork();
		if (pid2 == 0) {
			::execl(player_.c_str(), player_.c_str(), path_.c_str(), static_cast<char *>(nullptr));
			::_exit(127);
		}
		::_exit(0);
	} else if (pid > 0) {
		int status = 0;
		::waitpid(pid, &status, 0);
	}
#endif
}

} // namespace topscreen
