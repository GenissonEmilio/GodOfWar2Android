#!/usr/bin/env bash
# Runs the existing kernel/scheduler suite against the Android production libraries.
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${ANDROID_HOME:?Set ANDROID_HOME}"
compiler="$ANDROID_HOME/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang++"
build_dir="${GOW_BUILD_DIR:-artifacts/build/arm64}"
"$compiler" tests/android/kernel_probe.cpp PS2Recomp/ps2xTest/src/ps2_runtime_kernel_tests.cpp \
  PS2Recomp/ps2xTest/src/test_function_table.cpp -std=c++20 -O2 -static-libstdc++ -DUSE_SSE2NEON \
  -I "$build_dir/_deps/sse2neon-src" -I PS2Recomp/ps2xRuntime/include \
  -I PS2Recomp/ps2xRuntime/src/lib/Kernel -I PS2Recomp/ps2xIOP/include -I PS2Recomp/ps2xTest/include \
  -Wl,--start-group "$build_dir/PS2Recomp/ps2xRuntime/libps2_runtime.a" \
  "$build_dir/PS2Recomp/ps2xRuntime/liblibmpeg2.a" "$build_dir/PS2Recomp/ps2xIOP/libps2_iop.a" \
  "$build_dir/_deps/raylib-build/raylib/libraylib.a" -Wl,--end-group \
  -lEGL -lGLESv3 -lOpenSLES -llog -landroid -ldl -latomic -lm -o artifacts/kernel-probe
"$ANDROID_HOME/platform-tools/adb" push artifacts/kernel-probe /data/local/tmp/gow2-kernel-probe
"$ANDROID_HOME/platform-tools/adb" shell "timeout 55 /data/local/tmp/gow2-kernel-probe"
