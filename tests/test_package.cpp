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

/* Packages are versioned in releases/ and packed there by default. */
TEST(package_releases_folder)
{
	const std::string root = std::string(TOPSCREEN_SOURCE_DIR) + "/";

	const std::string packager = read_bytes(root + "scripts/package.sh");
	CHECK(!packager.empty());
	CHECK(packager.find("OUT=\"$ROOT/releases\"") != std::string::npos);

	/* Archives are verified before publishing (version, arch, data, hashes). */
	const std::string verifier = read_bytes(root + "scripts/verify-release.sh");
	CHECK(!verifier.empty());
	CHECK(verifier.find("loaded v$VER") != std::string::npos);
	CHECK(verifier.find("sha256sum") != std::string::npos);

	/* Index of every version, so the folder stays self-documenting. */
	const std::string index = read_bytes(root + "releases/README.md");
	CHECK(!index.empty());
	CHECK(index.find("topscreen-0.2.6-windows.zip") != std::string::npos);
	CHECK(index.find("topscreen-0.2.6-linux.tar.gz") != std::string::npos);
	CHECK(index.find("install.bat") != std::string::npos);

	/* CI publishes into releases/ and must not create GitHub Releases. */
	const std::string workflow = read_bytes(root + ".github/workflows/release.yml");
	CHECK(!workflow.empty());
	CHECK(workflow.find("releases/") != std::string::npos);
	CHECK(workflow.find("action-gh-release") == std::string::npos);
}

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

	/* The launchers must ship INSIDE the plugin folder (next to bin/ and
	 * data/), so nothing has to be moved after extracting the zip. */
	const std::string packager = read_bytes(root + "scripts/package.sh");
	CHECK(!packager.empty());
	CHECK(packager.find("install-windows.bat") != std::string::npos);
	CHECK(packager.find("uninstall-windows.bat") != std::string::npos);
	CHECK(packager.find("topscreen/install.bat") != std::string::npos);
	CHECK(packager.find("topscreen/uninstall.bat") != std::string::npos);

	const std::string workflow = read_bytes(root + ".github/workflows/release.yml");
	CHECK(!workflow.empty());
	CHECK(workflow.find("install-windows.bat") != std::string::npos);
	CHECK(workflow.find("uninstall-windows.bat") != std::string::npos);
	CHECK(workflow.find("pkg\\topscreen\\install.bat") != std::string::npos);
	CHECK(workflow.find("pkg\\topscreen\\uninstall.bat") != std::string::npos);
}

/* The installer must work from the release zip AND from the source repo,
 * and must always tell the user what to do when it cannot find a binary. */
TEST(package_windows_installer_has_fallbacks)
{
	const std::string install = read_bytes(std::string(TOPSCREEN_SOURCE_DIR) +
					       "/scripts/install-windows.bat");
	CHECK(!install.empty());

	/* data/ search: next to the script and one/two levels up (repo layout) */
	CHECK(install.find("%HERE%data") != std::string::npos);
	CHECK(install.find("%HERE%..\\data") != std::string::npos);
	CHECK(install.find("topscreen.effect") != std::string::npos);

	/* dll search: bin/, build output, x64/, out/ and the subtree */
	CHECK(install.find("bin\\64bit\\topscreen.dll") != std::string::npos);
	CHECK(install.find("build\\Release\\topscreen.dll") != std::string::npos);
	CHECK(install.find("build\\Debug\\topscreen.dll") != std::string::npos);
	CHECK(install.find("x64\\Release\\topscreen.dll") != std::string::npos);
	CHECK(install.find("out\\Release\\topscreen.dll") != std::string::npos);
	CHECK(install.find("dir /S /B") != std::string::npos);

	/* clear recovery guidance instead of a bare error */
	CHECK(install.find("Run as administrator") != std::string::npos);
	CHECK(install.find("was not found") != std::string::npos);
	CHECK(install.find("READY BUILD") != std::string::npos);
	CHECK(install.find("install-windows.ps1") != std::string::npos);
	CHECK(install.find("BUILD.md") != std::string::npos);

	/* drag and drop of a dll / portable folder */
	CHECK(install.find("obs-plugins") != std::string::npos);

	/* the uninstaller also explains when there was nothing to remove */
	const std::string uninstall =
		read_bytes(std::string(TOPSCREEN_SOURCE_DIR) + "/scripts/uninstall-windows.bat");
	CHECK(uninstall.find("Nothing was removed") != std::string::npos);
}
