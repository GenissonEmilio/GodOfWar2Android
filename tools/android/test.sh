#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p artifacts/java-tests
javac -d artifacts/java-tests android/app/src/main/java/io/github/genissonemilio/gow2/DiscImporter.java tests/android/DiscImporterTest.java
java -cp artifacts/java-tests io.github.genissonemilio.gow2.DiscImporterTest
python3 -m unittest discover -s tests/linux -v
