cmake_minimum_required(VERSION 3.20)
project(wow_export
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "wow.export — WoW game-data export tool (C++ port)"
)

# ── C++ standard ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Find vcpkg-managed dependencies ────────────────────────────
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# ── Executable ──────────────────────────────────────────────────
# Temporary entry point — will be replaced by the converted src/app.cpp
add_executable(wow_export src/main.cpp)

target_include_directories(wow_export PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/js
)

target_link_libraries(wow_export PRIVATE
    ZLIB::ZLIB
    OpenSSL::SSL
    OpenSSL::Crypto
    httplib::httplib
    pugixml::pugixml
    PNG::PNG
    WebP::webp
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    fmt::fmt
)

# ── Converted source files (added incrementally) ───────────────
add_subdirectory(src/js)
