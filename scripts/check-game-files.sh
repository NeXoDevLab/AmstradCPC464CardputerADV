#!/bin/sh
set -eu
cpc_root=$(cd "$(dirname "$0")/.." && pwd)
cpc_build=$(mktemp -d "${TMPDIR:-/tmp}/cpc-files.XXXXXX")
cpc_compiler=${CPC_CXX:-clang++}
cd "$cpc_root"
"$cpc_compiler" -std=c++17 -O1 -g -fsanitize=address,undefined \
  tests/host/game_files.cpp src/platform/GameFiles.cpp -o "$cpc_build/game-files"
"$cpc_build/game-files" "$cpc_build"
