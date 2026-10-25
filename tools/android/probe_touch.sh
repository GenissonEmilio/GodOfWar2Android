#!/usr/bin/env bash
# Injects movement + attacks into a foreground disposable game session (2772x1280).
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${ANDROID_HOME:?Set ANDROID_HOME}"
bt="$ANDROID_HOME/build-tools/35.0.0"
stage=artifacts/touch-probe
mkdir -p "$stage/classes" "$stage/dex"
cat > "$stage/AndroidManifest.xml" <<'MANIFEST'
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="io.github.genissonemilio.gow2.inputprobe"><uses-sdk android:minSdkVersion="29" android:targetSdkVersion="35"/><application android:label="GoW II input probe" android:debuggable="true"/><instrumentation android:name=".TouchProbe" android:targetPackage="io.github.genissonemilio.gow2.inputprobe"/></manifest>
MANIFEST
javac --release 17 -classpath "$ANDROID_HOME/platforms/android-35/android.jar" -d "$stage/classes" tests/android/TouchProbe.java
"$bt/d8" --lib "$ANDROID_HOME/platforms/android-35/android.jar" --min-api 29 --output "$stage/dex" "$stage"/classes/io/github/genissonemilio/gow2/inputprobe/*.class
"$bt/aapt" package -f -M "$stage/AndroidManifest.xml" -I "$ANDROID_HOME/platforms/android-35/android.jar" -F "$stage/base.apk"
(cd "$stage/dex"; zip -q ../base.apk classes.dex)
"$bt/zipalign" -f 4 "$stage/base.apk" "$stage/aligned.apk"
"$bt/apksigner" sign --ks artifacts/apk/debug.keystore --ks-pass pass:android --key-pass pass:android --out "$stage/probe.apk" "$stage/aligned.apk"
"$ANDROID_HOME/platform-tools/adb" install -r "$stage/probe.apk"
"$ANDROID_HOME/platform-tools/adb" shell am instrument -w io.github.genissonemilio.gow2.inputprobe/.TouchProbe
