#include "test_framework.hpp"

#include <fstream>
#include <sstream>
#include <string>

#ifndef TOPSCREEN_SOURCE_DIR
#define TOPSCREEN_SOURCE_DIR "."
#endif

namespace {

std::string read_bytes(const std::string &path)
{
	std::ifstream in(path, std::ios::binary);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

bool has_crlf(const std::string &text)
{
	return text.find("\r\n") != std::string::npos;
}

bool has_lone_lf(const std::string &text)
{
	for (size_t i = 0; i < text.size(); ++i) {
		if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r'))
			return true;
	}
	return false;
}

} // namespace

/* The Windows one-click launchers must exist, be CRLF (cmd.exe is picky
 * about LF-only files) and be packed by scripts/package.sh. */
TEST(package_windows_launchers_present)
{
	const std::string root = std::string(TOPSCREEN_SOURCE_DIR) + "/";

	const std::string install = read_bytes(root + "scripts/install-windows.bat");
	const std::string uninstall = read_bytes(root + "scripts/uninstall-windows.bat");

	CHECK(!install.empty());
	CHECK(!uninstall.empty());

	CHECK(has_crlf(install));
	CHECK(has_crlf(uninstall));
	CHECK(!has_lone_lf(install));
	CHECK(!has_lone_lf(uninstall));

	CHECK(install.rfind("@echo off", 0) == 0);
	CHECK(uninstall.rfind("@echo off", 0) == 0);

	/* The launchers must reach the release zip root as install/uninstall.bat. */
	const std::string packager = read_bytes(root + "scripts/package.sh");
	CHECK(!packager.empty());
	CHECK(packager.find("install-windows.bat") != std::string::npos);
	CHECK(packager.find("uninstall-windows.bat") != std::string::npos);
	CHECK(packager.find("install.bat") != std::string::npos);
	CHECK(packager.find("uninstall.bat") != std::string::npos);

	const std::string workflow = read_bytes(root + ".github/workflows/build-plugin.yml");
	CHECK(!workflow.empty());
	CHECK(workflow.find("install-windows.bat") != std::string::npos);
	CHECK(workflow.find("uninstall-windows.bat") != std::string::npos);
}
