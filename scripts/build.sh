#!/usr/bin/env bash
set -euo pipefail

CONFIG="${1:-debug}"
CONFIG_PRESET="linux-gcc-$CONFIG"
BUILD_PRESET="linux-$CONFIG"
OUT_DIR="out/build/$CONFIG_PRESET"

if [[ "${CLEAN:-0}" == "1" ]]; then
    echo "Cleaning $OUT_DIR ..."
    rm -rf "$OUT_DIR"
fi

cmake --preset "$CONFIG_PRESET"
cmake --build --preset "$BUILD_PRESET" --parallel
