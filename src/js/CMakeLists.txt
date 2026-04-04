# Per-file C++ targets will be added here as JS files are converted.
#
# Usage: after converting a JS file to real C++, add it to the CONVERTED_SOURCES
# list below so it is compiled into the wow_export executable.

target_sources(wow_export PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/install-type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MultiMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crc32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xml.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/subtitles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/blob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hashing/xxhash64.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/casc/content-flags.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/casc/locale-flags.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/casc/version-config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/casc/jenkins96.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/file-writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/png-writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/generics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mmap.cpp
)
