cmake_minimum_required(VERSION 3.20)

project(TopScreen VERSION 0.2.8 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(GNUInstallDirs)

option(TOPSCREEN_BUILD_PLUGIN "Build the OBS plugin"          ON)
option(TOPSCREEN_BUILD_TESTS  "Build the core unit tests"     ON)
option(ENABLE_QT              "Build the TopScreen Qt dock"   ON)
option(ENABLE_FRONTEND_API    "Use obs-frontend-api for dock" ON)

# ---------------------------------------------------------------------------
# Core (independent of OBS) - used by the plugin and the tests
# ---------------------------------------------------------------------------
set(TOPSCREEN_CORE_SOURCES
    src/core/easing.cpp
    src/core/spring.cpp
    src/core/geometry.cpp
    src/core/coord.cpp
    src/core/cursor.cpp
    src/core/click_fx.cpp
    src/core/zoom.cpp
    src/core/json.cpp
    src/core/preset.cpp
    src/core/key_overlay.cpp
    src/core/wav.cpp
    src/core/font_atlas.cpp
    src/core/stb_truetype_impl.cpp
    src/core/evdev.cpp
    src/core/highlighter.cpp
    src/core/cursor_atlas.cpp
    src/core/profiles.cpp
    src/core/mapping.cpp
    src/core/monitor.cpp
    src/core/pen.cpp
    src/core/image.cpp
    src/core/stb_image_impl.cpp
)

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
if(TOPSCREEN_BUILD_PLUGIN)

    # ---- libobs ----
    find_package(libobs QUIET)

    if(NOT libobs_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(LIBOBS QUIET IMPORTED_TARGET libobs)
        endif()
        if(LIBOBS_FOUND)
            message(STATUS "TopScreen: libobs via pkg-config")
            add_library(OBS::libobs ALIAS PkgConfig::LIBOBS)
        else()
            set(LIBOBS_INCLUDE_DIR "" CACHE PATH "libobs headers (obs-module.h)")
            set(LIBOBS_LIBRARY "" CACHE FILEPATH "libobs library (.so/.lib)")

            find_path(TOPSCREEN_LIBOBS_INCLUDE_DIR obs-module.h
                HINTS "${LIBOBS_INCLUDE_DIR}"
                PATHS /usr/include/obs /usr/local/include/obs)

            # LIBOBS_LIBRARY may be the library FILE itself (docs and the
            # installers pass it that way): HINTS only ever takes directories,
            # so use it directly when it exists.
            if(LIBOBS_LIBRARY AND EXISTS "${LIBOBS_LIBRARY}")
                set(TOPSCREEN_LIBOBS_LIBRARY "${LIBOBS_LIBRARY}")
            else()
                find_library(TOPSCREEN_LIBOBS_LIBRARY
                    NAMES obs libobs
                    HINTS "${LIBOBS_LIBRARY}"
                    PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
            endif()

            if(TOPSCREEN_LIBOBS_INCLUDE_DIR AND TOPSCREEN_LIBOBS_LIBRARY)
                message(STATUS "TopScreen: libobs via auto-detection (${TOPSCREEN_LIBOBS_LIBRARY})")
                add_library(OBS::libobs UNKNOWN IMPORTED)
                set_target_properties(OBS::libobs PROPERTIES
                    IMPORTED_LOCATION "${TOPSCREEN_LIBOBS_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${TOPSCREEN_LIBOBS_INCLUDE_DIR}")
            else()
                message(FATAL_ERROR
                    "libobs not found.\n"
                    "Install the OBS development package, or pass:\n"
                    "  -DLIBOBS_INCLUDE_DIR=/path/to/headers\n"
                    "  -DLIBOBS_LIBRARY=/path/to/libobs\n"
                    "Or use -DTOPSCREEN_BUILD_PLUGIN=OFF to build only the tests.\n"
                    "See docs/BUILD.md.")
            endif()
        endif()
    else()
        message(STATUS "TopScreen: using libobs from ${libobs_DIR}")
    endif()

    # ---- obs-frontend-api (optional; needed for the dock) ----
    set(TOPSCREEN_HAVE_FRONTEND OFF)
    if(ENABLE_FRONTEND_API)
        find_package(obs-frontend-api QUIET)
        if(obs-frontend-api_FOUND)
            set(TOPSCREEN_HAVE_FRONTEND ON)
        else()
            message(STATUS "TopScreen: obs-frontend-api not found, dock disabled")
        endif()
    endif()

    # ---- Qt6 (optional) ----
    set(TOPSCREEN_HAVE_QT OFF)
    if(ENABLE_QT AND TOPSCREEN_HAVE_FRONTEND)
        find_package(Qt6 QUIET COMPONENTS Widgets)
        if(Qt6_FOUND)
            set(TOPSCREEN_HAVE_QT ON)
        else()
            message(STATUS "TopScreen: Qt6 not found, dock disabled")
        endif()
    endif()

    # ---- sources ----
    set(TOPSCREEN_SOURCES
        src/plugin-main.cpp
        src/ts-filter.cpp
        src/ts-settings.cpp
        src/ts-properties.cpp
        src/ts-presets.cpp
        src/ts-registry.cpp
        src/ts-wallpapers.cpp
        src/render/ts-compositor.cpp
        src/render/ts-text.cpp
        src/render/ts-highlight.cpp
        src/input/ts-input.cpp
        src/input/ts-monitor.cpp
        src/input/ts-input-platform.cpp
        src/hotkeys/ts-hotkeys.cpp
        src/audio/ts-click-sound.cpp
        ${TOPSCREEN_CORE_SOURCES}
    )

    if(TOPSCREEN_HAVE_QT)
        list(APPEND TOPSCREEN_SOURCES src/ui/ts-dock.cpp)
    endif()

    add_library(topscreen MODULE ${TOPSCREEN_SOURCES})
    target_include_directories(topscreen PRIVATE src third_party)
    target_link_libraries(topscreen PRIVATE OBS::libobs)
    target_compile_definitions(topscreen PRIVATE TOPSCREEN_VERSION="${PROJECT_VERSION}")

    if(TOPSCREEN_HAVE_FRONTEND)
        target_compile_definitions(topscreen PRIVATE TOPSCREEN_HAVE_FRONTEND)
        target_link_libraries(topscreen PRIVATE OBS::obs-frontend-api)
    endif()

    if(TOPSCREEN_HAVE_QT)
        target_compile_definitions(topscreen PRIVATE TOPSCREEN_HAVE_QT)
        target_link_libraries(topscreen PRIVATE Qt6::Widgets)
        set_target_properties(topscreen PROPERTIES AUTOMOC ON)
    endif()

    if(WIN32)
        target_link_libraries(topscreen PRIVATE user32 winmm)
    elseif(UNIX)
        find_package(X11 QUIET)
        if(X11_FOUND)
            target_compile_definitions(topscreen PRIVATE TOPSCREEN_HAVE_X11)
            target_link_libraries(topscreen PRIVATE X11::X11)
            message(STATUS "TopScreen: X11 input backend enabled")
        endif()
        include(CheckIncludeFile)
        check_include_file(linux/input.h TOPSCREEN_HAVE_LINUX_INPUT)
        if(TOPSCREEN_HAVE_LINUX_INPUT)
            target_compile_definitions(topscreen PRIVATE TOPSCREEN_HAVE_EVDEV)
            message(STATUS "TopScreen: evdev input backend enabled (/dev/input)")
        endif()
    endif()

    set_target_properties(topscreen PROPERTIES
        PREFIX ""
        OUTPUT_NAME "topscreen"
        FOLDER "plugins"
    )

    # ---- installation ----
    if(UNIX AND NOT APPLE)
        install(TARGETS topscreen LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins")
        install(DIRECTORY data/ DESTINATION "${CMAKE_INSTALL_DATADIR}/obs/obs-plugins/topscreen")
    endif()
    if(WIN32)
        # A MODULE target needs LIBRARY DESTINATION even on Windows (the DLL
        # lands in RUNTIME); both are set so plain `cmake --install` works.
        install(TARGETS topscreen
            RUNTIME DESTINATION "obs-plugins/64bit"
            LIBRARY DESTINATION "obs-plugins/64bit")
        install(DIRECTORY data/ DESTINATION "obs-plugins/64bit/topscreen")
    endif()

    message(STATUS "TopScreen: Qt=${TOPSCREEN_HAVE_QT} frontend=${TOPSCREEN_HAVE_FRONTEND}")

else()
    message(STATUS "TopScreen: plugin disabled (tests only)")
endif()

# ---------------------------------------------------------------------------
# Core tests (do not depend on OBS)
# ---------------------------------------------------------------------------
if(TOPSCREEN_BUILD_TESTS)
    enable_testing()
    file(GLOB TOPSCREEN_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
    # ts-wallpapers.cpp does not depend on OBS and has its own unit coverage.
    add_executable(topscreen_tests ${TOPSCREEN_TEST_SOURCES} ${TOPSCREEN_CORE_SOURCES}
        src/ts-wallpapers.cpp)
    target_include_directories(topscreen_tests PRIVATE src third_party)
    target_compile_definitions(topscreen_tests PRIVATE
        TOPSCREEN_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
        TOPSCREEN_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME topscreen_tests COMMAND topscreen_tests)
endif()

message(STATUS "TopScreen: plugin=${TOPSCREEN_BUILD_PLUGIN} tests=${TOPSCREEN_BUILD_TESTS}")
