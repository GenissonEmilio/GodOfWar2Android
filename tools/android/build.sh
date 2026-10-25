#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
: "${ANDROID_HOME:?Set ANDROID_HOME to SDK 35 with NDK 28.2.13676358 and CMake 3.31.6}"
export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/28.2.13676358"
cd "$root/android"
exec ./gradlew --no-daemon :app:assembleDebug "$@"
