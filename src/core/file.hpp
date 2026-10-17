#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace topscreen::core {

/* Leitura de arquivos pequenos (fontes, atlas, presets, wallpapers). */
inline bool read_file_bytes(const char *path, std::vector<uint8_t> *out)
{
	if (!path || !out)
		return false;

	std::ifstream in(path, std::ios::binary);
	if (!in)
		return false;

	in.seekg(0, std::ios::end);
	const std::streampos end = in.tellg();
	if (end < 0 || end > 0x04000000)
		return false;
	in.seekg(0, std::ios::beg);

	out->resize(static_cast<size_t>(end));
	if (!out->empty())
		in.read(reinterpret_cast<char *>(out->data()), static_cast<std::streamsize>(out->size()));
	return in && !out->empty();
}

inline bool read_file_text(const char *path, std::string *out)
{
	if (!path || !out)
		return false;

	std::ifstream in(path);
	if (!in)
		return false;

	in.seekg(0, std::ios::end);
	const std::streampos end = in.tellg();
	if (end < 0 || end > 0x04000000)
		return false;
	in.seekg(0, std::ios::beg);

	out->resize(static_cast<size_t>(end));
	if (!out->empty())
		in.read(out->data(), static_cast<std::streamsize>(out->size()));
	return static_cast<bool>(in);
}

} // namespace topscreen::core
