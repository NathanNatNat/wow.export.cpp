# Prebuilt OpenSSL 3.x x64-windows binaries (MSVC)
# Exposes OpenSSL::SSL and OpenSSL::Crypto imported targets,
# matching what find_package(OpenSSL) would normally produce.

cmake_minimum_required(VERSION 3.20)

set(_OPENSSL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# ── Crypto ────────────────────────────────────────────────────────────────────
add_library(OpenSSL::Crypto SHARED IMPORTED GLOBAL)
set_target_properties(OpenSSL::Crypto PROPERTIES
    IMPORTED_LOCATION_RELEASE         "${_OPENSSL_ROOT}/bin/release/libcrypto-3-x64.dll"
    IMPORTED_IMPLIB_RELEASE           "${_OPENSSL_ROOT}/lib/release/libcrypto.lib"
    IMPORTED_LOCATION_DEBUG           "${_OPENSSL_ROOT}/bin/debug/libcrypto-3-x64.dll"
    IMPORTED_IMPLIB_DEBUG             "${_OPENSSL_ROOT}/lib/debug/libcrypto.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO  "${_OPENSSL_ROOT}/bin/release/libcrypto-3-x64.dll"
    IMPORTED_IMPLIB_RELWITHDEBINFO    "${_OPENSSL_ROOT}/lib/release/libcrypto.lib"
    INTERFACE_INCLUDE_DIRECTORIES     "${_OPENSSL_ROOT}/include"
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)

# ── SSL ───────────────────────────────────────────────────────────────────────
add_library(OpenSSL::SSL SHARED IMPORTED GLOBAL)
set_target_properties(OpenSSL::SSL PROPERTIES
    IMPORTED_LOCATION_RELEASE         "${_OPENSSL_ROOT}/bin/release/libssl-3-x64.dll"
    IMPORTED_IMPLIB_RELEASE           "${_OPENSSL_ROOT}/lib/release/libssl.lib"
    IMPORTED_LOCATION_DEBUG           "${_OPENSSL_ROOT}/bin/debug/libssl-3-x64.dll"
    IMPORTED_IMPLIB_DEBUG             "${_OPENSSL_ROOT}/lib/debug/libssl.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO  "${_OPENSSL_ROOT}/bin/release/libssl-3-x64.dll"
    IMPORTED_IMPLIB_RELWITHDEBINFO    "${_OPENSSL_ROOT}/lib/release/libssl.lib"
    INTERFACE_INCLUDE_DIRECTORIES     "${_OPENSSL_ROOT}/include"
    INTERFACE_LINK_LIBRARIES          OpenSSL::Crypto
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)

# Set OPENSSL_FOUND and version so find_package consumers are satisfied
set(OPENSSL_FOUND TRUE CACHE BOOL "" FORCE)
set(OPENSSL_VERSION "3.6.2" CACHE STRING "" FORCE)
set(OPENSSL_INCLUDE_DIR "${_OPENSSL_ROOT}/include" CACHE PATH "" FORCE)

# ── Copy DLLs to output dir at build time ─────────────────────────────────────
set(_OPENSSL_DLLS_RELEASE
    "${_OPENSSL_ROOT}/bin/release/libssl-3-x64.dll"
    "${_OPENSSL_ROOT}/bin/release/libcrypto-3-x64.dll"
)
set(_OPENSSL_DLLS_DEBUG
    "${_OPENSSL_ROOT}/bin/debug/libssl-3-x64.dll"
    "${_OPENSSL_ROOT}/bin/debug/libcrypto-3-x64.dll"
)

# These will be used by the main target via a post-build copy helper
set(OPENSSL_RUNTIME_DLLS_RELEASE "${_OPENSSL_DLLS_RELEASE}" CACHE INTERNAL "")
set(OPENSSL_RUNTIME_DLLS_DEBUG   "${_OPENSSL_DLLS_DEBUG}"   CACHE INTERNAL "")
