#!/usr/bin/env python3
"""Prepare a user's disc with the bundled compiler, without GTK."""
from pathlib import Path
import argparse
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1] / 'desktop'))
from model import Library,atomic_json
from pipeline import Pipeline
p=argparse.ArgumentParser();p.add_argument('--program',type=Path,required=True);p.add_argument('--data',type=Path,required=True);p.add_argument('--disc',type=Path,required=True);a=p.parse_args()
library=Library(a.data,a.program);library.directory.mkdir(parents=True,exist_ok=True)
exe=Pipeline(library.program,library.directory,library.game_environment(),lambda x:print(x,flush=True)).prepare(a.disc)
atomic_json(library.config,{'disc':str(a.disc.resolve()/'SCUS_974.81'),'serial':'SCUS-97481','name':'God of War II (USA)','executable':str(exe)})
print(exe,flush=True)
