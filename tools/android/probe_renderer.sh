#!/usr/bin/env bash
# Exercises the production renderer on an attached Android device with original synthetic input.
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${ANDROID_HOME:?Set ANDROID_HOME}"
compiler="$ANDROID_HOME/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang++"
"$compiler" tests/android/renderer_probe.cpp -std=c++20 -O2 -static-libstdc++ \
  -I PS2Recomp/ps2xRuntime/include \
  "${GOW_BUILD_DIR:-artifacts/build/arm64}/PS2Recomp/ps2xRuntime/libps2_runtime.a" \
  -lEGL -lGLESv3 -llog -landroid -o artifacts/renderer-probe
"$ANDROID_HOME/platform-tools/adb" push artifacts/renderer-probe /data/local/tmp/gow2-renderer-probe
"$ANDROID_HOME/platform-tools/adb" shell /data/local/tmp/gow2-renderer-probe
"$ANDROID_HOME/platform-tools/adb" shell GOW_GLES_FETCH_OFF=1 /data/local/tmp/gow2-renderer-probe
