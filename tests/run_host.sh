#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
roms="${1:?Uso: sh tests/run_host.sh DIRECTORIO_ROMS DSK CDT}"
test_build="$(mktemp -d /tmp/cpc-test.XXXXXX)"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -I tests/host \
  tests/host/peripherals.cpp src/core/FDC.cpp src/core/Tape.cpp src/core/Audio.cpp -o "$test_build/peripherals"
"$test_build/peripherals" "$test_build"
clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -I tests/host \
  tests/host/smoke.cpp src/core/Z80.cpp src/core/CRTC.cpp src/core/GA.cpp \
  src/core/PSG.cpp src/core/8255.cpp src/core/FDC.cpp src/core/Tape.cpp src/core/Audio.cpp \
  -o "$test_build/cpc-smoke"
"$test_build/cpc-smoke" "$roms" "$test_build/basic.ppm"
"$test_build/cpc-smoke" "$roms" "$test_build/dsk.ppm" "$2" '' 5000 3
"$test_build/cpc-smoke" "$roms" "$test_build/cdt.ppm" "$3" unused 40000 3
"$test_build/cpc-smoke" "$roms" "$test_build/loading-border.ppm" "$3" unused 3000 '' border
printf 'Capturas de las pruebas: %s\n' "$test_build"
