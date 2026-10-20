#pragma once

/**
 * TopScreen version, injected once by CMake (see TOPSCREEN_VERSION in
 * CMakeLists.txt). The fallback keeps the tests/translation layers buildable
 * outside the plugin target.
 */
#ifndef TOPSCREEN_VERSION
#define TOPSCREEN_VERSION "dev"
#endif

namespace topscreen {

inline constexpr const char *kVersion = TOPSCREEN_VERSION;

} // namespace topscreen
