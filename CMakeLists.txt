cmake_minimum_required(VERSION 3.20)

project(TopScreen VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(GNUInstallDirs)

option(ENABLE_QT            "Build the TopScreen Qt dock"        ON)
option(ENABLE_FRONTEND_API  "Use obs-frontend-api for the dock"  ON)
option(TOPSCREEN_BUILD_TESTS "Build the core unit tests"         ON)

# ---------------------------------------------------------------------------
# Nucleo (independente do OBS) - tambem usado pelos testes
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
)

# ---------------------------------------------------------------------------
# libobs
# ---------------------------------------------------------------------------
find_package(libobs QUIET)
if(NOT libobs_FOUND)
    message(STATUS "TopScreen: find_package(libobs) not found, trying pkg-config")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBOBS REQUIRED IMPORTED_TARGET libobs)
    add_library(OBS::libobs ALIAS PkgConfig::LIBOBS)
else()
    message(STATUS "TopScreen: using libobs from ${libobs_DIR}")
endif()

# ---------------------------------------------------------------------------
# obs-frontend-api (opcional)
# ---------------------------------------------------------------------------
set(TOPSCREEN_HAVE_FRONTEND OFF)
if(ENABLE_FRONTEND_API)
    find_package(obs-frontend-api QUIET)
    if(obs-frontend-api_FOUND)
        set(TOPSCREEN_HAVE_FRONTEND ON)
    else()
        message(STATUS "TopScreen: obs-frontend-api not found, dock disabled")
    endif()
endif()

# ---------------------------------------------------------------------------
# Qt6 (opcional, só com frontend)
# ---------------------------------------------------------------------------
set(TOPSCREEN_HAVE_QT OFF)
if(ENABLE_QT AND TOPSCREEN_HAVE_FRONTEND)
    find_package(Qt6 QUIET COMPONENTS Widgets)
    if(Qt6_FOUND)
        set(TOPSCREEN_HAVE_QT ON)
    else()
        message(STATUS "TopScreen: Qt6 not found, dock disabled")
    endif()
endif()

# ---------------------------------------------------------------------------
# Fontes do plugin
# ---------------------------------------------------------------------------
set(TOPSCREEN_SOURCES
    src/plugin-main.cpp
    src/ts-filter.cpp
    src/ts-settings.cpp
    src/ts-properties.cpp
    src/ts-presets.cpp
    src/render/ts-compositor.cpp
    src/input/ts-input.cpp
    src/input/ts-input-platform.cpp
    src/hotkeys/ts-hotkeys.cpp
    src/text/ts-sdf-font.cpp
    src/audio/ts-click-sound.cpp
    ${TOPSCREEN_CORE_SOURCES}
)

if(TOPSCREEN_HAVE_QT)
    list(APPEND TOPSCREEN_SOURCES src/ui/ts-dock.cpp)
endif()

add_library(topscreen MODULE ${TOPSCREEN_SOURCES})

target_include_directories(topscreen PRIVATE src)
target_link_libraries(topscreen PRIVATE OBS::libobs)

target_compile_definitions(topscreen PRIVATE
    TOPSCREEN_VERSION="${PROJECT_VERSION}"
)

if(TOPSCREEN_HAVE_FRONTEND)
    target_compile_definitions(topscreen PRIVATE TOPSCREEN_HAVE_FRONTEND)
    target_link_libraries(topscreen PRIVATE OBS::obs-frontend-api)
endif()

if(TOPSCREEN_HAVE_QT)
    target_compile_definitions(topscreen PRIVATE TOPSCREEN_HAVE_QT)
    target_link_libraries(topscreen PRIVATE Qt6::Widgets)
    set_target_properties(topscreen PROPERTIES AUTOMOC ON)
endif()

# ---------------------------------------------------------------------------
# Plataformas
# ---------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(topscreen PRIVATE user32)
elseif(UNIX)
    find_package(X11 QUIET)
    if(X11_FOUND)
        target_compile_definitions(topscreen PRIVATE TOPSCREEN_HAVE_X11)
        target_link_libraries(topscreen PRIVATE X11::X11)
        message(STATUS "TopScreen: X11 input backend enabled")
    endif()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBEVDEV QUIET libevdev)
        if(LIBEVDEV_FOUND)
            target_compile_definitions(topscreen PRIVATE TOPSCREEN_HAVE_EVDEV)
            target_link_libraries(topscreen PRIVATE PkgConfig::LIBEVDEV)
            message(STATUS "TopScreen: libevdev input backend enabled")
        endif()
    endif()
endif()

set_target_properties(topscreen PROPERTIES
    PREFIX ""
    OUTPUT_NAME "topscreen"
    FOLDER "plugins"
)

# ---------------------------------------------------------------------------
# Testes do nucleo (nao dependem do OBS)
# ---------------------------------------------------------------------------
if(TOPSCREEN_BUILD_TESTS)
    enable_testing()
    file(GLOB TOPSCREEN_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
    add_executable(topscreen_tests ${TOPSCREEN_TEST_SOURCES} ${TOPSCREEN_CORE_SOURCES})
    target_include_directories(topscreen_tests PRIVATE src)
    target_compile_definitions(topscreen_tests PRIVATE
        TOPSCREEN_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
    add_test(NAME topscreen_tests COMMAND topscreen_tests)
endif()

# ---------------------------------------------------------------------------
# Instalação
# ---------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    install(TARGETS topscreen LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins")
    install(DIRECTORY data/ DESTINATION "${CMAKE_INSTALL_DATADIR}/obs/obs-plugins/topscreen")
endif()

if(WIN32)
    install(TARGETS topscreen RUNTIME DESTINATION "obs-plugins/64bit")
    install(DIRECTORY data/ DESTINATION "obs-plugins/64bit/topscreen")
endif()

message(STATUS "TopScreen: Qt=${TOPSCREEN_HAVE_QT} frontend=${TOPSCREEN_HAVE_FRONTEND} tests=${TOPSCREEN_BUILD_TESTS}")
