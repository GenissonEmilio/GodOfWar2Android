// Exercise production GS register decoding and cached draw state with no game data.
#include "runtime/gs/gs_frontend.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
bool gowAndroidAsyncFinish(){return false;}
#include "capture_backend.h"
static void require(bool ok,const char *what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(){
    GS gs;auto backend=std::make_unique<CaptureBackend>();auto *capture=backend.get();gs.setRasterBackend(std::move(backend));
    gs.writeRegister(GS_REG_PRIM,GS_PRIM_POINT);
    auto kick=[&](unsigned x){gs.writeRegister(GS_REG_XYZ2,uint64_t(x*16)|(16ULL<<16));};
    for(unsigned i=0;i<1000;++i){gs.writeRegister(GS_REG_RGBAQ,0x3f80000080123456ULL);kick(i);}
    require(capture->draws.size()==1000,"repeated vertices");
    for(unsigned i=0;i<1000;++i)require(capture->draws[i].vertices[0].x==i && capture->draws[i].vertices[0].r==0x56,"vertex values");
    gs.writeRegister(GS_REG_TEX0_1,(5ULL<<26)|(4ULL<<30));gs.writeRegister(GS_REG_TEX1_1,1ULL<<5);kick(10);
    require(capture->draws.back().state.textureWidth==32 && capture->draws.back().state.textureHeight==16 && capture->draws.back().state.linearFilter,"derived texture state");
    gs.writeRegister(GS_REG_TEX0_2,(3ULL<<26)|(2ULL<<30));gs.writeRegister(GS_REG_PRIM,GS_PRIM_POINT|(1ULL<<9));kick(11);
    require(capture->draws.back().state.textureWidth==8 && capture->draws.back().state.textureHeight==4 && !capture->draws.back().state.linearFilter,"context switch");
    gs.writeRegister(GS_REG_FOGCOL,0x332211);gs.writeRegister(GS_REG_ALPHA_2,0x44);gs.writeRegister(GS_REG_COLCLAMP,1);kick(12);
    require(capture->draws.back().state.fogR==0x11 && capture->draws.back().state.fogB==0x33 && capture->draws.back().state.context.alpha==0x44 && capture->draws.back().state.colclamp==1,"global and context state");
    // PACKED A+D writes use the same invalidation path as direct/REGLIST writes.
    uint64_t packet[]{(1ULL<<60)|2,0xE,0x765432,GS_REG_FOGCOL,16ULL|(32ULL<<32),GS_REG_XYZ2};
    gs.processGIFPacket(reinterpret_cast<uint8_t*>(packet),sizeof(packet));
    require(capture->draws.back().state.fogR==0x32 && capture->draws.back().state.fogB==0x76,"packed A+D state");
    gs.reset();gs.writeRegister(GS_REG_PRIM,GS_PRIM_POINT);kick(13);
    require(capture->draws.back().state.textureWidth==1 && capture->draws.back().state.fogR==0 && !capture->draws.back().state.prim.ctxt,"reset state");
    std::puts("Cached GS state: changing vertices, textures, contexts, global state, packed A+D and reset: PASS");
}
