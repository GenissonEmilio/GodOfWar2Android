#!/usr/bin/env bash
# Package the locally built library using the Android SDK, without a second native build.
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
: "${ANDROID_HOME:?Set ANDROID_HOME}"
build_tools="$ANDROID_HOME/build-tools/35.0.0"
stage="$root/artifacts/apk"
mkdir -p "$stage/classes" "$stage/dex" "$stage/lib/arm64-v8a"
javac --release 17 -classpath "$ANDROID_HOME/platforms/android-35/android.jar" -d "$stage/classes" "$root"/android/app/src/main/java/io/github/genissonemilio/gow2/*.java
"$build_tools/d8" --lib "$ANDROID_HOME/platforms/android-35/android.jar" --min-api 29 --output "$stage/dex" "$stage"/classes/io/github/genissonemilio/gow2/*.class
python3 - "$root" "$stage" <<'PYMANIFEST'
from pathlib import Path
import sys
root,stage=map(Path,sys.argv[1:])
s=(root/'android/app/src/main/AndroidManifest.xml').read_text()
(stage/'AndroidManifest.xml').write_text(s.replace('<manifest ', '<manifest package="io.github.genissonemilio.gow2" ',1).replace('<application ', '<application android:debuggable="true" ',1))
PYMANIFEST
"$build_tools/aapt" package -f -M "$stage/AndroidManifest.xml" -I "$ANDROID_HOME/platforms/android-35/android.jar" -S "$root/android/app/src/main/res" --min-sdk-version 29 --target-sdk-version 35 --version-code 1 --version-name 0.1.0-dev -F "$stage/base.apk"
cp "${GOW_BUILD_DIR:-$root/artifacts/build/arm64}/PS2Recomp/ps2xRuntime/libps2EntryRunner.so" "$stage/lib/arm64-v8a/"
"$ANDROID_HOME/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip" --strip-debug "$stage/lib/arm64-v8a/libps2EntryRunner.so"
cd "$stage"
cp base.apk unsigned.apk
zip -q -r unsigned.apk lib
zip -q -j unsigned.apk dex/classes.dex
"$build_tools/zipalign" -P 16 -f 4 unsigned.apk aligned.apk
if [[ ! -f "$stage/debug.keystore" ]]; then keytool -genkeypair -keystore "$stage/debug.keystore" -storepass android -alias androiddebugkey -keypass android -dname 'CN=Android Debug,O=Android,C=US' -keyalg RSA -keysize 2048 -validity 10000; fi
"$build_tools/apksigner" sign --ks "$stage/debug.keystore" --ks-pass pass:android --key-pass pass:android --out "$root/artifacts/GodOfWar2Android-debug.apk" aligned.apk
"$build_tools/apksigner" verify "$root/artifacts/GodOfWar2Android-debug.apk"
