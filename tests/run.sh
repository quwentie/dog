#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/host
compiler="${CC:-gcc}"
flags=(-std=c99 -O2 -Wall -Wextra -Werror -Itests/stubs -IHardWare)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"$compiler" "${flags[@]}" -Dmain=pet_firmware_main -c User/main.c -o build/host/main.o
"$compiler" "${flags[@]}" tests/test_control.c HardWare/BlueTooth.c \
  HardWare/PetAction.c HardWare/Face_Config.c HardWare/Servo.c build/host/main.o -o build/host/test_control
build/host/test_control
