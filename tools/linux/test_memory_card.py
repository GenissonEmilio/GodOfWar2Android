#!/usr/bin/env python3
"""Exercise the actual native memory-card storage across separate processes."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'desktop'))
from model import Library
from pipeline import Pipeline
p=argparse.ArgumentParser();p.add_argument('--program',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
a.out=a.out.resolve();a.out.mkdir(parents=True,exist_ok=True)
library=Library(a.out,a.program);pipeline=Pipeline(library.program,library.directory,library.game_environment())
source=pipeline.source/'PS2Recomp/ps2xIOP/src/emulator/core'
fixture=a.out/'card_probe.cpp'
fixture.write_text(r'''
#include "memory_card.h"
#include <array>
#include <cstring>
#include <iostream>
int main(int argc,char**argv) {
 if(argc!=3)return 2;
 using ps2x::iop::detail::MemoryCard;
 MemoryCard card;card.insert(argv[2]);
 std::array<uint8_t,512> expected{}, actual{};
 for(unsigned i=0;i<expected.size();++i)expected[i]=(17*i+31)&255;
 card.setAddress(128);
 if(std::string(argv[1])=="write") {card.eraseBlock();card.write(expected.data(),expected.size());}
 else {card.read(actual.data(),actual.size());if(actual!=expected)return 3;}
 card.eject();std::cout<<"verified\n";
}
''')
tc=pipeline.bundle/'toolchain';exe=a.out/'card-probe'
args=[tc/'bin/g++','--sysroot='+str(tc/'sysroot'),'-B'+str(tc/'lib/gcc/x86_64-linux-gnu/13')+'/',
 '-std=c++20','-O2','-nostdinc++','-isystem',tc/'include/c++/13','-isystem',tc/'include/x86_64-linux-gnu/c++/13',
 '-isystem',tc/'include/c++/13/backward','-I'+str(source),fixture,source/'memory_card.cpp',
 '-static-libstdc++','-static-libgcc','-o',exe]
subprocess.run(list(map(str,args)),env=pipeline.compiler_env(),check=True)
card=a.out/'Mcd001.ps2'
for mode in ('write','read'):
 subprocess.run([str(exe),mode,str(card)],env=library.game_environment(),check=True)
assert card.stat().st_size==528*0x4000
report={'passed':True,'scope':'actual native card persistence across processes, synthetic page data; in-game save-menu behavior not established','card_bytes':card.stat().st_size}
(a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
