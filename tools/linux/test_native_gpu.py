#!/usr/bin/env python3
"""Run the actual GS context-sharing backend with an original synthetic texture."""
from pathlib import Path
import argparse
import json
import re
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'desktop'))
from model import Library
from pipeline import Pipeline
p=argparse.ArgumentParser();p.add_argument('--program',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
a.out=a.out.resolve();a.out.mkdir(parents=True,exist_ok=True)
library=Library(a.out,a.program)
class Probe(Pipeline):
 def run(self,args,cwd=None,env=None):
  args=list(args)
  if '-o' in args and '-c' not in args:args.append('-Wl,--wrap=main')
  return super().run(args,cwd,env)
pipeline=Probe(library.program,library.directory,library.game_environment(),print)
stage=pipeline.work/'gpu-probe';(stage/'output').mkdir(parents=True,exist_ok=True)
fixture=stage/'output/fixture.cpp'
header=pipeline.source/'PS2Recomp/ps2xRuntime/src/lib/gs/gpu/gs_gl.h'
fixture.write_text('#include '+json.dumps(str(header))+'\n'+r'''
#include "raylib.h"
#include "rlgl.h"
#include "runtime/gs/gs_shared_present.h"
#include "ps2_runtime.h"
#include <future>
#include <thread>
#include <iostream>
extern const uint32_t g_ps2RecompiledFunctionTableBase = 0;
extern const uint32_t g_ps2RecompiledFunctionTableEnd = 0x01000000;
extern const uint32_t g_ps2RecompiledFunctionTableSlotCount = 0x00400000;
PS2Runtime::RecompiledFunction g_ps2RecompiledFunctionTable[0x00400000] = {};
extern "C" int __wrap_main(int,char**) {
 SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(320,240,"GodOfWar2PC native GPU probe");
 if(!IsWindowReady())return 10;
 GSSharedPresent::CaptureHostContext();
 std::promise<bool> ready;auto available=ready.get_future();std::promise<void> release;auto finish=release.get_future();
 std::thread worker([&] {
   GSGL::Context context;std::string error;
   if(!context.Create(error)) {std::cerr<<error<<std::endl;ready.set_value(false);return;}
   const auto &gl=context.gl();
   std::cout<<"Native GPU: "<<gl.GetString(0x1F01)<<std::endl;
   unsigned texture=0;gl.GenTextures(1,&texture);gl.BindTexture(0x0DE1,texture);
   const unsigned char pixels[]{230,41,55,255,230,41,55,255,230,41,55,255,230,41,55,255};
   gl.TexImage2D(0x0DE1,0,0x8058,2,2,0,0x1908,0x1401,pixels);
   gl.TexParameteri(0x0DE1,0x2801,0x2600);gl.TexParameteri(0x0DE1,0x2800,0x2600);
   bool ok=gl.hwRaster&&GSGL::PublishSharedTexture(gl,texture,2,2,1,0,0,1);
   std::cerr<<"[probe] hw="<<gl.hwRaster<<" publish="<<ok<<" error="<<gl.GetError()<<std::endl;
   gl.Finish();ready.set_value(ok);finish.wait();
   GSGL::DestroyShared(gl);gl.DeleteTextures(1,&texture);
 });
 bool ok=available.get();GSSharedPresent::Frame frame;
 ok=ok&&GSSharedPresent::Acquire(frame)&&frame.width==2&&frame.height==2;
 std::cerr<<"[probe] acquire="<<ok<<" tex="<<frame.texture<<" width="<<frame.width<<std::endl;
 if(ok) {
   BeginDrawing();ClearBackground(BLACK);
   DrawTexturePro(Texture2D{frame.texture,2,2,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8},{0,0,2,2},{0,0,320,240},{0,0},0,WHITE);
   rlDrawRenderBatchActive();Image image=LoadImageFromScreen();Color pixel=GetImageColor(image,160,120);
   std::cerr<<"[probe] pixel="<<int(pixel.r)<<","<<int(pixel.g)<<","<<int(pixel.b)<<std::endl;
   ok=pixel.r==230&&pixel.g==41&&pixel.b==55;UnloadImage(image);EndDrawing();
 }
 GSSharedPresent::ShutdownHost();release.set_value();worker.join();CloseWindow();
 if(ok) std::cout<<"native GPU texture shared and verified"<<std::endl;
 return ok?0:12;
}
''')
names=sorted(set(re.findall(r'\b[A-Za-z_][A-Za-z_0-9]*_0x[0-9a-fA-F]+\b',(pipeline.source/'overrides/gow2_overrides.cpp').read_text())))
with fixture.open('a') as output:
 for name in names:output.write('void '+name+'(uint8_t*,R5900Context*,PS2Runtime*) {}\n')
exe=pipeline.compile(stage)
result=subprocess.run([str(exe)],env=library.game_environment(),capture_output=True,text=True,timeout=30)
(a.out/'native-gpu.log').write_text(result.stdout+result.stderr)
print(result.stdout+result.stderr)
assert result.returncode==0,result.returncode
assert 'native GPU texture shared and verified' in result.stdout
(a.out/'report.json').write_text(json.dumps({'passed':True,'scope':'actual OpenGL 4.6 GS worker and GPU texture sharing; synthetic image only'},indent=2)+'\n')
