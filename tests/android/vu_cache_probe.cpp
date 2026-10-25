// Synthetic VU programs only: exercise native hits, misses and code replacement.
#include "runtime/ps2_vu1_native.h"
#include "runtime/ps2_memory.h"
#include "runtime/gs/gs_frontend.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <cstdlib>
bool gowAndroidAsyncFinish(){return false;}
static unsigned nativeCalls=0;
static uint32_t one(VU1NativeCtx &n){++nativeCalls;n.s->vi[1]=1;return kVu1nEnded|((n.s->pc+16u)&0x3fffu);}
static uint32_t two(VU1NativeCtx &n){++nativeCalls;n.s->vi[1]=2;return kVu1nEnded|((n.s->pc+16u)&0x3fffu);}
static uint32_t scalarLower=0;
static uint32_t scalar(VU1NativeCtx &n){++nativeCalls;VU1NativeAccess::callLower(n,0,scalarLower,0x2ffu);return kVu1nEnded|24u;}
static std::array<uint32_t,4> program(unsigned value){return {0x10010000u|value,0x400002ffu,0x8000033cu,0x000002ffu};}
int main(){
    PS2Memory memory;if(!memory.initialize())return 1;
    GS gs;VU1Interpreter vu;
    unsigned cases=0;
    for(unsigned pc : {0u,504u,512u,16368u}){
        auto a=program(1),b=program(2);
        vu1nRegisterBlock(vu1n::blockKey(pc,a.data(),4),one,2,nullptr);
        vu1nRegisterBlock(vu1n::blockKey(pc,b.data(),4),two,2,nullptr);
        for(unsigned value : {1u,1u,2u,2u,3u,3u,1u,2u}){
            const auto code=program(value);const uint64_t before=memory.getVU1CodeGeneration();
            const bool changed=std::memcmp(memory.getVU1Code()+pc,code.data(),16)!=0;
            uint32_t upload[5]{0x4a020000u|(pc/8)};std::memcpy(upload+1,code.data(),16);
            memory.processVIF1Data(reinterpret_cast<uint8_t*>(upload),sizeof(upload));
            if(memory.getVU1CodeGeneration()!=before+(changed?1u:0u))return 2;
            const unsigned calls=nativeCalls;
            vu.execute(memory.getVU1Code(),PS2_VU1_CODE_SIZE,memory.getVU1Data(),PS2_VU1_DATA_SIZE,gs,&memory,pc,0,0,64);
            if(vu.state().vi[1]!=int(value) || nativeCalls!=calls+(value!=3)){
                std::fprintf(stderr,"FAIL pc=%u value=%u actual=%d native=%u\n",pc,value,vu.state().vi[1],nativeCalls-calls);return 3;
            }
            ++cases;
        }
    }
    // DIV/WAITQ and EFU/WAITP exercise the compact scalar timing records.
    for(unsigned which=0;which<2;++which){
        scalarLower=which?0x80000fbdu:0x80020bbcu; // ERSQRT vf1.x / DIV vf1.x, vf2.x
        const uint32_t code[]{scalarLower,0x2ffu,which?0x800007bfu:0x800003bfu,0x400002ffu,0x8000033cu,0x2ffu};
        std::memcpy(memory.getVU1Code(),code,sizeof(code));memory.markVU1CodeModified();
        vu1nRegisterBlock(vu1n::blockKey(0,code,6),scalar,3,nullptr);
        VU1Interpreter exact,native;
        for(auto *unit:{&exact,&native}){unit->state().vf[1][0]=which?4.0f:18.0f;unit->state().vf[2][0]=3.0f;}
        setenv("GOW_VU1_NATIVE_OFF","1",1);
        exact.execute(memory.getVU1Code(),PS2_VU1_CODE_SIZE,memory.getVU1Data(),PS2_VU1_DATA_SIZE,gs,&memory,0,0,0,128);
        unsetenv("GOW_VU1_NATIVE_OFF");const auto before=nativeCalls;
        native.execute(memory.getVU1Code(),PS2_VU1_CODE_SIZE,memory.getVU1Data(),PS2_VU1_DATA_SIZE,gs,&memory,0,0,0,128);
        const auto &a=exact.state(),&b=native.state();
        if((which?a.p!=0.5f:a.q!=6.0f) || nativeCalls!=before+1 || a.q!=b.q || a.p!=b.p || a.cycles!=b.cycles || a.pc!=b.pc || a.status!=b.status){
            std::fprintf(stderr,"Scalar mismatch which=%u Q=%f/%f P=%f/%f cycles=%llu/%llu pc=%u/%u\n",which,a.q,b.q,a.p,b.p,(unsigned long long)a.cycles,(unsigned long long)b.cycles,a.pc,b.pc);return 4;
        }
        ++cases;
    }
    std::printf("%u VU cache cases: native replacement, interpreter fallback, repeated MPG, bitmap boundaries and scalar timing; 64-byte slots: PASS\n",cases);
}
