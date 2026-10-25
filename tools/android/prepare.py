#!/usr/bin/env python3
"""Extract and statically translate an owned USA v1.01 disc. Never uploads data."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/desktop'))
from import_disc import import_disc

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--disc', type=Path, required=True)
    p.add_argument('--recompiler', type=Path, required=True)
    p.add_argument('--native-cache', type=Path)
    a = p.parse_args()
    # Use the same validated transactional dual-layer importer as the PC port.
    disc = ROOT / 'artifacts/discs/game'
    if not disc.exists(): import_disc(a.disc.resolve(), disc)
    expected = 'fd403603be46dea70d130695820d28b3f31de6d7e48484a5f006398a605f7dd6'
    if hashlib.sha256((disc / 'SCUS_974.81').read_bytes()).hexdigest() != expected:
        raise ValueError('Cached disc has an incompatible executable; no files were changed')
    requested = a.disc.resolve()
    if not requested.is_file(): raise FileNotFoundError(requested)
    gen = ROOT / 'generated'
    gen.mkdir(exist_ok=True)
    cfg = (ROOT / 'config/recomp.template.toml').read_text().replace('@ELF@', str(disc / 'SCUS_974.81')).replace('@MAP@', str(ROOT / 'config/funcmap.csv')).replace('@OUT@', str(gen / 'output') + '/')
    (gen / 'config.toml').write_text(cfg)
    subprocess.run([str(a.recompiler.resolve()), str(gen / 'config.toml')], check=True)
    if a.native_cache:
        shutil.copytree(a.native_cache, gen / 'native', dirs_exist_ok=True)
    print(f'Prepared game data: {disc}\nGenerated sources: {gen}')
if __name__ == '__main__': main()
