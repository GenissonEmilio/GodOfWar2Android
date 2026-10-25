#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${ANDROID_HOME:?Set ANDROID_HOME}"
build_dir="${GOW_BUILD_DIR:-artifacts/build/arm64}"
compiler="$ANDROID_HOME/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang++"
adb="$ANDROID_HOME/platform-tools/adb"
"$compiler" tests/android/vu_capture_probe.cpp -std=c++20 -O3 -static-libstdc++ -DUSE_SSE2NEON \
  -I "$build_dir/_deps/sse2neon-src" -I PS2Recomp/ps2xRuntime/include \
  "$build_dir/PS2Recomp/ps2xRuntime/libps2_runtime.a" -lEGL -lGLESv3 -llog -landroid -o artifacts/vu_capture-probe
"$adb" push artifacts/vu_capture-probe /data/local/tmp/gow2-vu-capture-probe
capture="/data/local/tmp/gow2-vu-synthetic-$$.bin"
trap '"$adb" shell rm -f "$capture" "$capture.arm"' EXIT
"$adb" shell "timeout 30 /data/local/tmp/gow2-vu-capture-probe record $capture"
"$adb" shell "timeout 30 /data/local/tmp/gow2-vu-capture-probe replay $capture" 2>&1 | tee artifacts/vu_capture-replay.log
rg -q '\[vu1n-replay\] 16 records, 0 differ' artifacts/vu_capture-replay.log
