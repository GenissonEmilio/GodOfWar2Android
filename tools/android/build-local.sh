#!/usr/bin/env bash
# Build with the SDK's CMake/Ninja and package with aapt/d8. No Gradle installation is required.
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
: "${ANDROID_HOME:?Set ANDROID_HOME to SDK 35 with NDK 28.2.13676358 and CMake 3.31.6}"
cmake="$ANDROID_HOME/cmake/3.31.6/bin/cmake"
args=(-S "$root" -B "$root/artifacts/build/arm64" -G Ninja
    "-DCMAKE_MAKE_PROGRAM=$ANDROID_HOME/cmake/3.31.6/bin/ninja"
    "-DCMAKE_TOOLCHAIN_FILE=$ANDROID_HOME/ndk/28.2.13676358/build/cmake/android.toolchain.cmake"
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release)
if [[ -n "${GOW_RAYLIB_SOURCE:-}" ]]; then args+=("-DFETCHCONTENT_SOURCE_DIR_RAYLIB=$GOW_RAYLIB_SOURCE"); fi
"$cmake" "${args[@]}"
"$cmake" --build "$root/artifacts/build/arm64" --target ps2EntryRunner -j "${GOW_BUILD_JOBS:-2}"
exec "$root/tools/android/package-local.sh"
