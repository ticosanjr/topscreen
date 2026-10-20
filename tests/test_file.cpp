#include "core/file.hpp"

#include "test_framework.hpp"

#include <filesystem>
#include <fstream>
#include <string>

using namespace topscreen::core;

/* C7: paths are UTF-8 everywhere. A user-selected file may contain accents
 * (especially on Windows); the helpers must convert it to a native path. */
TEST(file_read_write_with_utf8_name)
{
	const std::string name = "topscreen-\xC3\xA7\xC3\xA3o.txt"; // "ção"
	const std::filesystem::path path =
		std::filesystem::temp_directory_path() / utf8_path(name.c_str());

	{
		std::ofstream out(path, std::ios::binary);
		REQUIRE(static_cast<bool>(out));
		out << "hello";
	}

	const std::u8string u8 = path.u8string();
	const std::string utf8(reinterpret_cast<const char *>(u8.data()), u8.size());

	std::string text;
	CHECK(read_file_text(utf8.c_str(), &text));
	CHECK_EQ(text, std::string("hello"));

	std::filesystem::remove(path);
}
