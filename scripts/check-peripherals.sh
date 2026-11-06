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
