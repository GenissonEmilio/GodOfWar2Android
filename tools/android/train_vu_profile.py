#!/usr/bin/env python3
"""Train local VU IR-PGO from an owned capture, checking every replay hash.

Uses an existing ARM64 build's original compile commands and runtime archive.
Does not modify the APK/build objects or upload captures to a network service.
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[2]


def arguments(entry):
    return list(entry['arguments']) if 'arguments' in entry else shlex.split(entry['command'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'artifacts/build/arm64')
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'artifacts/vu-combat-ir.profdata')
    parser.add_argument('--jobs', type=int, default=3, choices=range(1, 9))
    args = parser.parse_args()
    build, capture, output = args.build_dir.resolve(), args.capture.resolve(), args.output.resolve()
    if not capture.is_file():
        parser.error(f'Capture not found: {capture}')
    if not os.environ.get('ANDROID_HOME'):
        parser.error('Set ANDROID_HOME to the Android SDK used by the build')
    sdk = Path(os.environ['ANDROID_HOME']).resolve()
    adb = str(sdk / 'platform-tools/adb')
    entries = json.loads((build / 'compile_commands.json').read_text())
    parts = [e for e in entries if re.fullmatch(r'vu1n_part\d+\.cpp', Path(e['file']).name)]
    cores = [e for e in entries if Path(e['file']).name == 'ps2_vu1_core.cpp']
    if not parts or len(cores) != 1:
        parser.error('Use one configured ARM64 runtime build with generated native VU sources')
    compiler = Path(arguments(cores[0])[0])
    if 'clang++' not in compiler.name or 'aarch64' not in str(arguments(cores[0])):
        parser.error('Training requires the Android NDK ARM64 Clang build')
    archive = build / 'PS2Recomp/ps2xRuntime/libps2_runtime.a'
    if not archive.is_file():
        parser.error('Build ps2EntryRunner before training')
    folder = build / 'vu-pgo-training'
    folder.mkdir(exist_ok=True)
    profile_flags = ['-flto=thin', '-fprofile-generate']

    def compile_entry(entry):
        name = Path(entry['file']).stem
        command = arguments(entry)
        # Keep the original source path: internal-function profile names include
        # the translation-unit name. Do not overwrite production build objects.
        command[command.index('-o') + 1] = str(folder / (name + '.o'))
        # A previously optimized build must not use an older profile while training.
        command = [a for a in command if not a.startswith(('-fprofile-use=', '-fprofile-instr-use='))]
        command += profile_flags
        log = folder / (name + '.log')
        with log.open('w') as stream:
            subprocess.run(command, cwd=entry['directory'], check=True, stdout=stream, stderr=subprocess.STDOUT)
        return folder / (name + '.o')

    selected = parts + cores
    print(f'Training {len(parts)} native VU units; logs: {folder}', flush=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        objects = list(pool.map(compile_entry, selected))
    binary = folder / 'vu-replay-training'
    target_flags = [a for a in arguments(cores[0]) if a.startswith(('--target=', '--sysroot='))]
    command = [str(compiler)] + target_flags + ['-std=c++20', '-O3', '-ffp-contract=off', '-static-libstdc++',
               '-DUSE_SSE2NEON', '-I' + str(build / '_deps/sse2neon-src'),
               '-I' + str(ROOT / 'PS2Recomp/ps2xRuntime/include'), str(ROOT / 'tests/android/vu_replay_main.cpp')]
    command += [str(obj) for obj in objects]
    command += [str(archive), '-lEGL', '-lGLESv3', '-llog', '-landroid', '-o', str(binary)]
    command += profile_flags + [f'-Wl,--thinlto-jobs={args.jobs}']
    with (folder / 'link.log').open('w') as stream:
        subprocess.run(command, check=True, stdout=stream, stderr=subprocess.STDOUT)
    remote = '/data/local/tmp/gow2-vu-training-' + uuid.uuid4().hex[:12]
    raw = folder / 'combat.profraw'
    try:
        subprocess.run([adb, 'shell', 'mkdir', '-p', remote], check=True)
        subprocess.run([adb, 'push', str(binary), remote + '/replay'], check=True)
        subprocess.run([adb, 'push', str(capture), remote + '/capture.bin'], check=True)
        result = subprocess.run([adb, 'shell', f'LLVM_PROFILE_FILE={remote}/combat.profraw {remote}/replay {remote}/capture.bin'],
                                check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
        (folder / 'replay.log').write_text(result.stdout)
        verified = re.search(r'\[vu1n-replay\] (\d+) records, (\d+) differ', result.stdout)
        if not verified or int(verified[1]) == 0 or int(verified[2]) != 0:
            raise RuntimeError('Capture did not replay exactly; profile was not accepted. See ' + str(folder / 'replay.log'))
        subprocess.run([adb, 'pull', remote + '/combat.profraw', str(raw)], check=True)
        output.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([str(compiler.parent / 'llvm-profdata'), 'merge', '-sparse', str(raw), '-o', str(output)], check=True)
    finally:
        # Only this run's random directory on the attached test device.
        subprocess.run([adb, 'shell', 'rm', '-rf', remote], check=False)
    files = [Path(e['file']) for e in selected] + [ROOT / 'PS2Recomp/ps2xRuntime/include/runtime/ps2_vu1_native.h',
             ROOT / 'PS2Recomp/ps2xRuntime/src/lib/vu/ps2_vu1_native.inl']
    metadata = {'capture_sha256': hashlib.sha256(capture.read_bytes()).hexdigest(), 'records': int(verified[1]),
                'compiler': subprocess.check_output([str(compiler), '--version'], text=True).splitlines()[0],
                'sources': {str(p.relative_to(ROOT) if p.is_relative_to(ROOT) else p):
                            hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
    Path(str(output) + '.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'Accepted {verified[1]} bit-exact replays; local IR-PGO profile: {output}')


if __name__ == '__main__':
    try:
        main()
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, RuntimeError) as error:
        raise SystemExit(f'Training failed: {error if isinstance(error, RuntimeError) else type(error).__name__}; inspect the training logs') from None
