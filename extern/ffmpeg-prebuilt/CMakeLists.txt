# Prebuilt FFmpeg LGPL shared x64 binaries (BtbN build).
# Populated by scripts/setup-ffmpeg.ps1 — run that before configuring CMake.
# Exposes FFmpeg::avcodec, FFmpeg::avformat, FFmpeg::avutil, FFmpeg::swscale.

cmake_minimum_required(VERSION 3.20)

set(_FFMPEG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT EXISTS "${_FFMPEG_ROOT}/lib/avcodec.lib")
    message(FATAL_ERROR
        "FFmpeg prebuilt not found in extern/ffmpeg-prebuilt/.\n"
        "Run:  pwsh scripts/setup-ffmpeg.ps1\n"
        "then re-run CMake configure.")
endif()

# ── Locate versioned DLLs ─────────────────────────────────────────────────────
file(GLOB _AVUTIL_DLL      "${_FFMPEG_ROOT}/bin/avutil-*.dll")
file(GLOB _SWRESAMPLE_DLL  "${_FFMPEG_ROOT}/bin/swresample-*.dll")
file(GLOB _AVCODEC_DLL     "${_FFMPEG_ROOT}/bin/avcodec-*.dll")
file(GLOB _AVFORMAT_DLL    "${_FFMPEG_ROOT}/bin/avformat-*.dll")
file(GLOB _SWSCALE_DLL     "${_FFMPEG_ROOT}/bin/swscale-*.dll")

# ── avutil ────────────────────────────────────────────────────────────────────
add_library(FFmpeg::avutil SHARED IMPORTED GLOBAL)
set_target_properties(FFmpeg::avutil PROPERTIES
    IMPORTED_LOCATION             "${_AVUTIL_DLL}"
    IMPORTED_IMPLIB               "${_FFMPEG_ROOT}/lib/avutil.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${_FFMPEG_ROOT}/include"
)

# ── swresample (dependency of avcodec) ────────────────────────────────────────
add_library(FFmpeg::swresample SHARED IMPORTED GLOBAL)
set_target_properties(FFmpeg::swresample PROPERTIES
    IMPORTED_LOCATION             "${_SWRESAMPLE_DLL}"
    IMPORTED_IMPLIB               "${_FFMPEG_ROOT}/lib/swresample.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${_FFMPEG_ROOT}/include"
    INTERFACE_LINK_LIBRARIES      FFmpeg::avutil
)

# ── avcodec ───────────────────────────────────────────────────────────────────
add_library(FFmpeg::avcodec SHARED IMPORTED GLOBAL)
set_target_properties(FFmpeg::avcodec PROPERTIES
    IMPORTED_LOCATION             "${_AVCODEC_DLL}"
    IMPORTED_IMPLIB               "${_FFMPEG_ROOT}/lib/avcodec.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${_FFMPEG_ROOT}/include"
    INTERFACE_LINK_LIBRARIES      "FFmpeg::avutil;FFmpeg::swresample"
)

# ── avformat ──────────────────────────────────────────────────────────────────
add_library(FFmpeg::avformat SHARED IMPORTED GLOBAL)
set_target_properties(FFmpeg::avformat PROPERTIES
    IMPORTED_LOCATION             "${_AVFORMAT_DLL}"
    IMPORTED_IMPLIB               "${_FFMPEG_ROOT}/lib/avformat.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${_FFMPEG_ROOT}/include"
    INTERFACE_LINK_LIBRARIES      "FFmpeg::avutil;FFmpeg::avcodec"
)

# ── swscale ───────────────────────────────────────────────────────────────────
add_library(FFmpeg::swscale SHARED IMPORTED GLOBAL)
set_target_properties(FFmpeg::swscale PROPERTIES
    IMPORTED_LOCATION             "${_SWSCALE_DLL}"
    IMPORTED_IMPLIB               "${_FFMPEG_ROOT}/lib/swscale.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${_FFMPEG_ROOT}/include"
    INTERFACE_LINK_LIBRARIES      FFmpeg::avutil
)

# ── Expose all runtime DLLs for post-build copy ───────────────────────────────
file(GLOB _FFMPEG_RUNTIME_DLLS
    "${_FFMPEG_ROOT}/bin/av*.dll"
    "${_FFMPEG_ROOT}/bin/sw*.dll"
    "${_FFMPEG_ROOT}/bin/postproc*.dll"
)
set(FFMPEG_RUNTIME_DLLS "${_FFMPEG_RUNTIME_DLLS}" CACHE INTERNAL "")
