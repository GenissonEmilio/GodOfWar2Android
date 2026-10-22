#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
docker build --cpuset-cpus=0,1 -t gow2-linux-builder -f tools/linux/Dockerfile .
docker run --rm --init --cpus=2 -u "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$root:/src:z" gow2-linux-builder bash -c \
  'cmake -S PS2Recomp -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_FLAGS="-msse4.1 -fno-strict-aliasing" -DPS2X_ENABLE_DEBUG_UI=OFF -DPS2X_BUILD_ANALYZER=OFF -DPS2X_BUILD_TEST=OFF -DPS2X_BUILD_STUDIO=OFF -DPS2X_ENABLE_AGRESSIVE_LOGS=OFF -DPS2X_ENABLE_IOP_RPC_TRACE=OFF -DPS2X_ENABLE_RUNNER_PCH=OFF && nice -n 10 cmake --build build/cmake --target ps2_runtime ps2_recomp -j2 && nice -n 10 python3 tools/linux/prepare_compiler.py && python3 tools/linux/collect_licenses.py --kind native'
# The GTK packager is intentionally separate from the C++ compilation kit.
docker build --cpuset-cpus=0,1 -t gow2-desktop-builder -f tools/desktop/release/Dockerfile .
docker run --rm --init --cpus=2 -u "$(id -u):$(id -g)" -e HOME=/tmp -v "$root:/src:z" \
  gow2-desktop-builder python3 -m PyInstaller --noconfirm --clean \
  --distpath artifacts/frozen --workpath artifacts/pyinstaller tools/desktop/release/launcher.spec
docker run --rm --init --cpus=2 -u "$(id -u):$(id -g)" -v "$root:/src:z" \
  gow2-desktop-builder python3 tools/linux/collect_licenses.py --kind launcher
mkdir -p artifacts/compiler-check
ln -sfn ../compiler artifacts/compiler-check/compiler
docker run --rm --init --cpus=2 --network=none -u "$(id -u):$(id -g)" -e HOME=/tmp -v "$root:/src:z" \
  gow2-linux-builder python3 tools/linux/test_compiler.py --program /src/artifacts/compiler-check
python3 tools/desktop/release/package_linux.py
