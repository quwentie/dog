#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/arm
compiler="${ARM_CC:-arm-none-eabi-gcc}"
flags=(-std=c99 -mcpu=cortex-m3 -mthumb -O2 -ffreestanding -Wall -Wextra -Werror
  -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -IStart -ILibrary -IUser -IHardWare)
if [[ -n "${NEWLIB_INCLUDE:-}" ]]; then
  flags+=(-I"$NEWLIB_INCLUDE")
fi
for source in HardWare/BlueTooth.c HardWare/PetAction.c HardWare/Face_Config.c HardWare/Servo.c User/main.c; do
  output="$(basename "${source%.c}")"
  "$compiler" "${flags[@]}" -c "$source" -o "build/arm/$output.o"
done
printf '%s\n' 'PASS: 5 Cortex-M3 object files compiled with production STM32 headers (no link/HEX)'
