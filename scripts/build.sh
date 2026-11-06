#!/bin/sh
set -eu
cpc_root=$(cd "$(dirname "$0")/.." && pwd)
cpc_cli=${CPC_ARDUINO_CLI:-arduino-cli}
cpc_fqbn='m5stack:esp32:m5stack_cardputer:PSRAM=disabled,FlashMode=qio,FlashSize=8M,PartitionScheme=default_8MB,USBMode=hwcdc,CDCOnBoot=cdc'
"$cpc_cli" compile --fqbn "$cpc_fqbn" --build-path "$cpc_root/.build-verified" "$cpc_root"
