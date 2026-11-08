#!/bin/sh
set -eu
cpc_root=$(cd "$(dirname "$0")/.." && pwd)
cpc_build=$(mktemp -d "${TMPDIR:-/tmp}/cpc-peripherals.XXXXXX")
cpc_compiler=${CPC_CXX:-clang++}
cd "$cpc_root"
"$cpc_compiler" -std=c++17 -O1 -g -fsanitize=address,undefined -I tests/host \
  tests/host/peripherals.cpp src/core/FDC.cpp src/core/Tape.cpp src/core/Audio.cpp \
  -o "$cpc_build/peripherals"
"$cpc_build/peripherals" "$cpc_build"
"$cpc_compiler" -std=c++17 -O1 -g -fsanitize=address,undefined -I tests/host \
  tests/host/dsk_boot.cpp src/core/FDC.cpp -o "$cpc_build/dsk-boot"
"$cpc_build/dsk-boot" "$cpc_build"
"$cpc_compiler" -std=c++17 -O1 -g -fsanitize=address,undefined \
  tests/host/display_scaler.cpp -o "$cpc_build/display-scaler"
"$cpc_build/display-scaler"
"$cpc_compiler" -std=c++17 -O1 -g -fsanitize=address,undefined \
  tests/host/recording_rle.cpp -o "$cpc_build/recording-rle"
"$cpc_build/recording-rle"
