cmake_minimum_required(VERSION 3.20)
project(wow_export_cpp
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "wow.export.cpp — WoW game-data export tool (C++ port)"
)

# ── Force x64 architecture ──────────────────────────────────────
# Reject any non-64-bit configuration early.
if(CMAKE_SIZEOF_VOID_P AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR
        "wow.export.cpp requires a 64-bit (x64) build. "
        "Detected pointer size: ${CMAKE_SIZEOF_VOID_P} bytes. "
        "Please select an x64 toolchain / preset."
    )
endif()

# On MSVC, verify the target processor architecture.
if(MSVC AND DEFINED CMAKE_VS_PLATFORM_NAME AND NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    message(FATAL_ERROR
        "wow.export.cpp requires x64. Current platform: ${CMAKE_VS_PLATFORM_NAME}. "
        "Use an x64 preset or pass -A x64."
    )
endif()

# ── C++ standard ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Build configuration defaults ───────────────────────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# ── Visual Studio: set wow_export_cpp as the startup project ──────
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT wow_export_cpp)

# ── Git submodule check ─────────────────────────────────────────
# Verify all required submodules are initialized and populated.
# Each entry maps a submodule directory to a sentinel file that
# must exist when the submodule has been checked out.
set(_REQUIRED_SUBMODULES
    "extern/cpp-httplib  CMakeLists.txt"
    "extern/glad         LICENSE"
    "extern/glfw         CMakeLists.txt"
    "extern/glm          CMakeLists.txt"
    "extern/imgui        imgui.cpp"
    "extern/json         CMakeLists.txt"
    "extern/libwebp      CMakeLists.txt"
    "extern/miniaudio    miniaudio.h"
    "extern/nanosvg      CMakeLists.txt"
    "extern/pugixml      CMakeLists.txt"
    "extern/spdlog       CMakeLists.txt"
    "extern/stb          stb_image.h"
    "extern/zlib         CMakeLists.txt"
)

set(_MISSING_SUBMODULES "")
foreach(_entry IN LISTS _REQUIRED_SUBMODULES)
    # Split "dir  sentinel" into two variables.
    string(REGEX MATCH "^([^ ]+)" _subdir "${_entry}")
    string(REGEX REPLACE "^[^ ]+[ ]+" "" _sentinel "${_entry}")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${_subdir}/${_sentinel}")
        list(APPEND _MISSING_SUBMODULES "${_subdir}")
    endif()
endforeach()

if(_MISSING_SUBMODULES)
    string(REPLACE ";" "\n    " _formatted "${_MISSING_SUBMODULES}")
    message(FATAL_ERROR
        "The following git submodules are not initialized:\n"
        "    ${_formatted}\n"
        "Please run:\n"
        "    git submodule update --init --recursive\n"
        "then re-run CMake."
    )
endif()

# ════════════════════════════════════════════════════════════════
#  Dependencies (all from extern/ git submodules)
# ════════════════════════════════════════════════════════════════

# ── GLFW (windowing) ───────────────────────────────────────────
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glfw EXCLUDE_FROM_ALL)

# ── GLAD2 (OpenGL 4.6 core loader, regenerated every build) ───
set(GLAD_SOURCES_DIR "${CMAKE_SOURCE_DIR}/extern/glad" CACHE STRING "" FORCE)
add_subdirectory(extern/glad/cmake EXCLUDE_FROM_ALL)
glad_add_library(glad_gl_core_46 STATIC REPRODUCIBLE API gl:core=4.6)

# ── Dear ImGui (docking branch, compiled as a static library) ──
add_library(imgui STATIC
    extern/imgui/imgui.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/backends/imgui_impl_glfw.cpp
    extern/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    extern/imgui
    extern/imgui/backends
)
target_link_libraries(imgui PUBLIC glfw glad_gl_core_46)
# Enable docking and multiviewport support
target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLAD2
)

# ── GLM (math, header-only) ───────────────────────────────────
set(GLM_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glm EXCLUDE_FROM_ALL)

# ── nlohmann/json (JSON parsing) ──────────────────────────────
set(JSON_BuildTests     OFF CACHE BOOL "" FORCE)
set(JSON_Install        OFF CACHE BOOL "" FORCE)
set(JSON_MultipleHeaders OFF CACHE BOOL "" FORCE)
set(JSON_CI             OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/json EXCLUDE_FROM_ALL)

# ── spdlog (logging, bundles fmt) ─────────────────────────────
set(SPDLOG_BUILD_EXAMPLE    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS_HO   OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH      OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_ALL        OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL          OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/spdlog EXCLUDE_FROM_ALL)

# ── cpp-httplib (HTTP client/server, header-only) ─────────────
set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_TEST    OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/cpp-httplib EXCLUDE_FROM_ALL)

# ── zlib (compression) ────────────────────────────────────────
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING  OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
set(ZLIB_INSTALL        OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
add_subdirectory(extern/zlib EXCLUDE_FROM_ALL)
# zlib's CMake names its target "zlibstatic" for static builds.
# Fix interface include dirs (zlib's CMakeLists.txt uses raw paths
# that CMake rejects for installed/exported targets).
set_target_properties(zlibstatic PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ""
)
target_include_directories(zlibstatic PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/extern/zlib>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/extern/zlib>
)

# ── libwebp (WebP image I/O) ─────────────────────────────────
set(WEBP_BUILD_ANIM_UTILS OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_CWEBP      OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_DWEBP      OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_GIF2WEBP   OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_IMG2WEBP   OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_VWEBP      OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPINFO   OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPMUX    OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_LIBWEBPMUX OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_EXTRAS     OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBP_JS    OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_FUZZTEST   OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/libwebp EXCLUDE_FROM_ALL)

# ── pugixml (XML parsing) ────────────────────────────────────
set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/pugixml EXCLUDE_FROM_ALL)

# ── stb (stb_image / stb_image_write, header-only) ───────────
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE extern/stb)

# ── nanosvg (SVG parsing, header-only) ────────────────────────
add_library(nanosvg INTERFACE)
target_include_directories(nanosvg INTERFACE extern/nanosvg/src)

# ── miniaudio (audio, header-only single file) ────────────────
add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE extern/miniaudio)

# ════════════════════════════════════════════════════════════════
#  Main executable
# ════════════════════════════════════════════════════════════════

add_executable(wow_export_cpp src/app.cpp)

target_include_directories(wow_export_cpp PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/js
)

target_link_libraries(wow_export_cpp PRIVATE
    # Windowing & rendering
    glfw
    glad_gl_core_46
    imgui
    # Math
    glm::glm
    # Data formats
    nlohmann_json::nlohmann_json
    pugixml::pugixml
    # Compression
    zlibstatic
    # Image I/O
    webp
    stb
    nanosvg
    # Audio
    miniaudio
    # HTTP
    httplib::httplib
    # Logging (bundles fmt)
    spdlog::spdlog
)

# Platform-specific link dependencies
if(WIN32)
    target_link_libraries(wow_export_cpp PRIVATE ws2_32 crypt32 dwmapi)
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(wow_export_cpp PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif()

# ── Copy runtime resources to the build output directory ───────
# The application locates resources relative to the executable directory
# under a "data" subdirectory (constants::DATA_DIR = <exe_dir>/data).
# After every build, mirror the project's resource files next to the binary.
add_custom_command(TARGET wow_export_cpp POST_BUILD
    # src/shaders/  →  <exe_dir>/data/shaders/
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/shaders"
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/shaders"

    # src/fa-icons/  →  <exe_dir>/data/fa-icons/
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/fa-icons"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/fa-icons"
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/fa-icons"

    # src/images/  →  <exe_dir>/data/images/
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/images"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/images"
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/images"

    # src/fonts/  →  <exe_dir>/data/fonts/
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/fonts"
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/fonts"

    # resources/  →  <exe_dir>/data/icons/
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/icons"

    # src/default_config.jsonc  →  <exe_dir>/data/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/default_config.jsonc"
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/default_config.jsonc"

    # src/app.css  →  <exe_dir>/data/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/app.css"
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/app.css"

    # src/whats-new.html  →  <exe_dir>/data/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/whats-new.html"
        "$<TARGET_FILE_DIR:wow_export_cpp>/data/whats-new.html"

    COMMENT "Copying runtime resources to data/ directory"
)

# ── Converted source files (added incrementally) ───────────────
add_subdirectory(src/js)

# ════════════════════════════════════════════════════════════════
#  Installer executable (standalone console application — disabled by default)
#  Set -DWOW_EXPORT_BUILD_INSTALLER=ON to build.
# ════════════════════════════════════════════════════════════════
option(WOW_EXPORT_BUILD_INSTALLER "Build the installer executable" OFF)

if(WOW_EXPORT_BUILD_INSTALLER)
    add_executable(wow_export_installer installer/installer.cpp)

    target_include_directories(wow_export_installer PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(wow_export_installer PRIVATE
        nlohmann_json::nlohmann_json
        zlibstatic
    )

    if(MSVC)
        target_compile_options(wow_export_installer PRIVATE /W4)
    else()
        target_compile_options(wow_export_installer PRIVATE -Wall -Wextra)
    endif()

    set_target_properties(wow_export_installer PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "wow.export.cpp.installer"
    )
endif()

# ════════════════════════════════════════════════════════════════
#  Updater executable (standalone application — disabled by default)
#  Set -DWOW_EXPORT_BUILD_UPDATER=ON to build.
# ════════════════════════════════════════════════════════════════
option(WOW_EXPORT_BUILD_UPDATER "Build the updater executable" OFF)

if(WOW_EXPORT_BUILD_UPDATER)
    add_executable(wow_export_updater updater/updater.cpp)

    target_include_directories(wow_export_updater PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    if(MSVC)
        target_compile_options(wow_export_updater PRIVATE /W4)
    else()
        target_compile_options(wow_export_updater PRIVATE -Wall -Wextra)
    endif()

    set_target_properties(wow_export_updater PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "wow.export.cpp.updater"
    )
endif()
