#!/usr/bin/env bash
# Read-only verification of the game identity on an already prepared test device.
set -euo pipefail
: "${ANDROID_HOME:?Set ANDROID_HOME}"
adb="$ANDROID_HOME/platform-tools/adb"
"$adb" devices -l
"$adb" shell getprop ro.product.model
"$adb" shell getprop ro.build.version.release
"$adb" shell getprop ro.product.cpu.abi
"$adb" shell dumpsys SurfaceFlinger | rg -m 1 'GLES:'
"$adb" shell sha256sum /sdcard/Android/data/io.github.genissonemilio.gow2/files/game/SCUS_974.81
"$adb" shell wc -c /sdcard/Android/data/io.github.genissonemilio.gow2/files/game/PART1.PAK /sdcard/Android/data/io.github.genissonemilio.gow2/files/game/PART2.PAK
