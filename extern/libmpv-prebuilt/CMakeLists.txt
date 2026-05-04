# Prebuilt libmpv dev package (shinchiro build).
# Populated by scripts/setup-libmpv.ps1 — run that before configuring CMake.
# Exposes libmpv::libmpv imported target.

cmake_minimum_required(VERSION 3.20)

set(_LIBMPV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT EXISTS "${_LIBMPV_ROOT}/lib/mpv.lib")
    message(FATAL_ERROR
        "libmpv prebuilt not found in extern/libmpv-prebuilt/.\n"
        "Run:  pwsh scripts/setup-libmpv.ps1\n"
        "then re-run CMake configure.")
endif()

# ── libmpv ────────────────────────────────────────────────────────────────────
add_library(libmpv::libmpv SHARED IMPORTED GLOBAL)
set_target_properties(libmpv::libmpv PROPERTIES
    IMPORTED_LOCATION             "${_LIBMPV_ROOT}/bin/libmpv-2.dll"
    IMPORTED_IMPLIB               "${_LIBMPV_ROOT}/lib/mpv.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${_LIBMPV_ROOT}/include"
)

# ── Expose runtime DLL for post-build copy ────────────────────────────────────
set(LIBMPV_RUNTIME_DLLS "${_LIBMPV_ROOT}/bin/libmpv-2.dll" CACHE INTERNAL "")
