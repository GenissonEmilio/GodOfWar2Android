#!/usr/bin/env bash
# Exercise ordering/readback on the production queue using synthetic commands only.
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${ANDROID_HOME:?Set ANDROID_HOME}"
compiler="$ANDROID_HOME/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang++"
"$compiler" tests/android/threaded_backend_probe.cpp -std=c++20 -O2 -static-libstdc++ \
  -I PS2Recomp/ps2xRuntime/include "${GOW_BUILD_DIR:-artifacts/build/arm64}/PS2Recomp/ps2xRuntime/libps2_runtime.a" \
  -lEGL -lGLESv3 -llog -landroid -o artifacts/threaded-probe
"$ANDROID_HOME/platform-tools/adb" push artifacts/threaded-probe /data/local/tmp/gow2-threaded-probe
"$ANDROID_HOME/platform-tools/adb" shell /data/local/tmp/gow2-threaded-probe
