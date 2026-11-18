#!/usr/bin/env bash
# Port/eame: this game as an unsigned iOS app, with the kit
# (port-eame-kit/tools/ios_env.sh). Runs on a Mac with Xcode; the kit's
# action runs it on GitHub. Change the CMake options and the target's name.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
export PORTEAME_WORK="${PORTEAME_WORK:-$root/build/ios}"
source "$root/port-eame-kit/tools/ios_env.sh"

porteame_build_sdl3

cmake -S "$root" -B "$PORTEAME_WORK/build" "${porteame_ios_cmake[@]}" \
    -DSDL3_DIR="$PORTEAME_SDL3_DIR" \
    -DCMAKE_EXE_LINKER_FLAGS="$PORTEAME_IOS_LINK"
cmake --build "$PORTEAME_WORK/build" --target my_game --parallel "$porteame_jobs"

binary="$PORTEAME_WORK/build/my_game"
porteame_check_binary "$binary"
porteame_package "$root/porteame.json" "$binary"
