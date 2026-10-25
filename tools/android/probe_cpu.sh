#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${ANDROID_HOME:?Set ANDROID_HOME}"
compiler="$ANDROID_HOME/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang++"
build_dir="${GOW_BUILD_DIR:-artifacts/build/arm64}"
for probe in iop_clock vu_cache gs_memory frame_clock fp_control; do
  "$compiler" "tests/android/${probe}_probe.cpp" -std=c++20 -O3 -static-libstdc++ -DUSE_SSE2NEON \
    -I "$build_dir/_deps/sse2neon-src" -I PS2Recomp/ps2xRuntime/include -I PS2Recomp/ps2xIOP/include -I native \
    "$build_dir/PS2Recomp/ps2xRuntime/libps2_runtime.a" \
    -lEGL -lGLESv3 -llog -landroid -o "artifacts/${probe}-probe"
  "$ANDROID_HOME/platform-tools/adb" push "artifacts/${probe}-probe" "/data/local/tmp/gow2-${probe}-probe"
  "$ANDROID_HOME/platform-tools/adb" shell "timeout 45 /data/local/tmp/gow2-${probe}-probe"
done
