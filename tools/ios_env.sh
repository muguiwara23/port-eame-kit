# Port/eame kit: what a game's iOS build script sources (bash).
#
#     kit="$root/port-eame-kit"
#     source "$kit/tools/ios_env.sh"
#     porteame_build_sdl3
#     cmake -S "$root" -B "$PORTEAME_WORK/build" "${porteame_ios_cmake[@]}" \
#         -DSDL3_DIR="$PORTEAME_SDL3_DIR" -DCMAKE_EXE_LINKER_FLAGS="$PORTEAME_IOS_LINK" ...
#     cmake --build "$PORTEAME_WORK/build" --target game --parallel "$porteame_jobs"
#     porteame_check_binary "$PORTEAME_WORK/build/game"
#     porteame_package "$root/porteame.json" "$PORTEAME_WORK/build/game"
#
# Runs on a Mac with Xcode (GitHub's macOS runners, Intel or Apple silicon:
# both build the arm64 iOS app). Settings, from the environment:
#   PORTEAME_WORK     where everything is built (default: build/ios under the game)
#   PORTEAME_VERSION  the version in the .ipa's name (default: the sheet's)
#   PORTEAME_ICON     a square picture for the app's icon (the action decrypts it)
#   PORTEAME_MINIMUM_IOS  the oldest iOS (default 16.0)

if [[ "$(uname -s)" != Darwin ]]; then
    echo "Port/eame: iOS builds need macOS and Xcode." >&2
    return 2 2>/dev/null || exit 2
fi

PORTEAME_KIT="${PORTEAME_KIT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
PORTEAME_WORK="${PORTEAME_WORK:-$PWD/build/ios}"
PORTEAME_MINIMUM_IOS="${PORTEAME_MINIMUM_IOS:-16.0}"
porteame_jobs="$(sysctl -n hw.ncpu 2>/dev/null || echo 3)"
mkdir -p "$PORTEAME_WORK"
PORTEAME_WORK="$(cd "$PORTEAME_WORK" && pwd)"
export PORTEAME_KIT PORTEAME_WORK PORTEAME_MINIMUM_IOS

# CMake for iOS arm64. Executables stay plain files (CMAKE_MACOSX_BUNDLE OFF):
# package_ipa.py makes the .app. Try-compiles link nothing (no signing).
porteame_ios_cmake=(-G Ninja -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos
                    -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET="$PORTEAME_MINIMUM_IOS"
                    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_BUILD_TYPE=Release
                    -DCMAKE_MACOSX_BUNDLE=OFF)

xcodebuild -version
echo "iOS SDK: $(xcrun --sdk iphoneos --show-sdk-version) at $(xcrun --sdk iphoneos --show-sdk-path)"

# SDL3, static, for iOS, from its pinned release (checked by its SHA-256).
# Sets PORTEAME_SDL3_DIR (for -DSDL3_DIR), PORTEAME_SDL3_SOURCE (its license
# is there) and PORTEAME_IOS_LINK (the frameworks to link, OpenGL ES too).
porteame_build_sdl3() {
    local version=3.4.16
    local sha256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
    local url="https://github.com/libsdl-org/SDL/releases/download/release-${version}/SDL3-${version}.tar.gz"
    local deps="$PORTEAME_WORK/deps"
    local install="$deps/sdl3"
    PORTEAME_SDL3_SOURCE="$deps/SDL3-$version"
    mkdir -p "$deps"
    if [[ ! -f "$PORTEAME_SDL3_SOURCE/LICENSE.txt" ]]; then
        curl -fsSL "$url" -o "$deps/sdl3.tar.gz"
        echo "$sha256  $deps/sdl3.tar.gz" | shasum -a 256 -c -
        tar -xzf "$deps/sdl3.tar.gz" -C "$deps"
    fi
    if [[ ! -f "$install/lib/libSDL3.a" ]]; then
        cmake -S "$PORTEAME_SDL3_SOURCE" -B "$deps/sdl3-build" "${porteame_ios_cmake[@]}" \
            -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF \
            -DCMAKE_INSTALL_PREFIX="$install"
        cmake --build "$deps/sdl3-build" --parallel "$porteame_jobs"
        cmake --install "$deps/sdl3-build"
    fi
    PORTEAME_SDL3_DIR="$install/lib/cmake/SDL3"
    # The frameworks the static SDL3 records for this SDK, plus OpenGL ES.
    PORTEAME_IOS_LINK="$(python3 - "$install/lib/pkgconfig/sdl3.pc" <<'PY'
import sys
words = [w for line in open(sys.argv[1]) if line.startswith(("Libs:", "Libs.private:"))
         for w in line.split(":", 1)[1].split()]
out = []
for i, w in enumerate(words):
    if w in ("-framework", "-weak_framework") and i + 1 < len(words):
        out += [w, words[i + 1]]
    elif w.startswith("-Wl,") and "framework" in w:
        out.append(w)
    elif w.startswith("-l") and w not in ("-lSDL3", "-lm"):
        out.append(w)
if not any("UIKit" in w for w in out):
    sys.exit("sdl3.pc lists no iOS frameworks: " + " ".join(words))
if "OpenGLES" not in " ".join(out):
    out += ["-framework", "OpenGLES"]
print(" ".join(out))
PY
)"
    export PORTEAME_SDL3_DIR PORTEAME_SDL3_SOURCE PORTEAME_IOS_LINK
    echo "SDL3 $version: $PORTEAME_SDL3_DIR"
    echo "link: $PORTEAME_IOS_LINK"
}

# The linked executable is arm64 for iOS, at the minimum version.
porteame_check_binary() {
    local binary="$1"
    [[ -f "$binary" ]] || { echo "Port/eame: no executable at $binary" >&2; return 1; }
    lipo -info "$binary"
    otool -l "$binary" | awk '/LC_BUILD_VERSION/{f=1} f&&/(platform|minos|sdk)/{print} f&&/ntools/{f=0}'
    if ! otool -l "$binary" | awk '/LC_BUILD_VERSION/{f=1} f&&/platform/{print $2; exit}' | grep -qiE '^(2|IOS)$'; then
        echo "Port/eame: $binary is not an iOS executable" >&2
        return 1
    fi
}

# The unsigned .ipa, under $PORTEAME_WORK/release. Extra arguments go to
# package_ipa.py (like --extra DIR).
porteame_package() {
    local sheet="$1" binary="$2"
    shift 2
    local args=(--config "$sheet" --binary "$binary" --out "$PORTEAME_WORK/release")
    [[ -n "${PORTEAME_VERSION:-}" ]] && args+=(--version "$PORTEAME_VERSION")
    [[ -n "${PORTEAME_ICON:-}" ]] && args+=(--icon "$PORTEAME_ICON")
    python3 "$PORTEAME_KIT/tools/package_ipa.py" "${args[@]}" "$@"
}
