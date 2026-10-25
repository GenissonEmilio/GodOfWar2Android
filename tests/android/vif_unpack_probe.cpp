// Compare production VIF unpack with an independent scalar reference.
#include "runtime/ps2_memory.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>
bool gowAndroidAsyncFinish(){return false;}
int main(){
    PS2Memory memory;if(!memory.initialize())return 1;
    unsigned cases=0;
    for(unsigned cl : {1u,2u,4u,17u,255u})for(unsigned wl : {1u,2u,4u,17u,255u}){
        if(cl<wl)continue;
        for(unsigned start : {0u,1u,1000u,1023u})for(unsigned count : {1u,2u,31u,255u,256u})for(unsigned mode : {0u,1u,2u,3u})for(bool masked : {false,true}){
            std::array<uint32_t,4096> expected;expected.fill(0xdeadbeef);
            std::memcpy(memory.getVU1Data(),expected.data(),sizeof(expected));
            std::array<uint32_t,4> row{11,22,33,44};
            for(unsigned i=0;i<4;++i){memory.vif1_regs.row[i]=row[i];memory.vif1_regs.col[i]=100+i;}
            memory.vif1_regs.mode=mode;memory.vif1_regs.mask=0xe4e4e4e4;memory.vif1_regs.cycle=cl|(wl<<8);
            memory.vif1_regs.tops=29;
            std::vector<uint32_t> packet(1+count*4);
            packet[0]=((masked?0x7c:0x6c)<<24)|((count&255)<<16)|0x8000|start;
            for(unsigned i=0;i<count*4;++i)packet[i+1]=i*0x01030507u+0x80000000;
            for(unsigned i=0;i<count;++i){
                unsigned dest=((start+29)+(i/wl)*cl+i%wl)&1023;
                for(unsigned lane=0;lane<4;++lane){
                    unsigned select=masked?lane:0;
                    uint32_t value=packet[1+4*i+lane];
                    if(select==0){if(mode==1 || mode==2)value+=row[lane];if(mode==2 || mode==3)row[lane]=value;}
                    else if(select==1)value=row[lane];
                    else if(select==2)value=100+std::min(i%wl,3u);
                    else continue;
                    expected[dest*4+lane]=value;
                }
            }
            memory.processVIF1Data(reinterpret_cast<uint8_t*>(packet.data()),packet.size()*4);
            if(std::memcmp(expected.data(),memory.getVU1Data(),sizeof(expected)) || std::memcmp(row.data(),memory.vif1_regs.row,16)){
                std::fprintf(stderr,"FAIL cl=%u wl=%u start=%u count=%u mode=%u mask=%d\n",cl,wl,start,count,mode,masked);return 2;
            }
            ++cases;
        }
    }
    std::printf("%u V4-32 cases: STCYCL skips, address wrap, TOPS, NUM=0, modes and mask fallback: PASS\n",cases);
}
