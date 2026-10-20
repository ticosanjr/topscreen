#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace topscreen::core {

/** UTF-8 path -> filesystem path (wide on Windows, bytes elsewhere). */
inline std::filesystem::path utf8_path(const char *path)
{
#if defined(__cpp_char8_t)
	/* C++20: u8path is deprecated, but constructing from u8string keeps the
	 * UTF-8 -> native conversion on Windows. */
	return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t *>(path)));
#else
	return std::filesystem::u8path(path);
#endif
}

/* Reading small files (fonts, atlas, presets, wallpapers). */
inline bool read_file_bytes(const char *path, std::vector<uint8_t> *out)
{
	if (!path || !out)
		return false;

	std::ifstream in(utf8_path(path), std::ios::binary);
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

	std::ifstream in(utf8_path(path));
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
