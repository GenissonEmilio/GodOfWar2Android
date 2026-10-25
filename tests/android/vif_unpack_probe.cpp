// Compare production VIF unpack with an independent scalar reference.
#include "runtime/ps2_memory.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>
bool gowAndroidAsyncFinish(){return false;}
int main(){
    PS2Memory memory;if(!memory.initialize())return 1;
    unsigned cases=0;
    for(unsigned cl : {1u,2u,4u,17u,255u})for(unsigned wl : {1u,2u,4u,17u,255u}){
        if(cl<wl)continue;
        for(unsigned vn : {1u,2u,3u})for(unsigned vl : {0u,1u,2u})for(bool zero : {false,true})for(unsigned start : {0u,1u,1000u,1023u})for(unsigned count : {1u,2u,31u,255u,256u})for(unsigned mode : {0u,1u,2u,3u})for(bool masked : {false,true}){
            std::array<uint32_t,4096> expected;expected.fill(0xdeadbeef);
            std::memcpy(memory.getVU1Data(),expected.data(),sizeof(expected));
            std::array<uint32_t,4> row{11,22,33,44};
            for(unsigned i=0;i<4;++i){memory.vif1_regs.row[i]=row[i];memory.vif1_regs.col[i]=100+i;}
            memory.vif1_regs.mode=mode;memory.vif1_regs.mask=0xe4e4e4e4;memory.vif1_regs.cycle=cl|(wl<<8);
            memory.vif1_regs.tops=29;
            const unsigned bytes=4u>>vl;
            const unsigned components=vn+1;
            std::vector<uint8_t> packet(4+((count*components*bytes+3u)&~3u));
            const uint32_t command=((0x60|(vn<<2)|vl|(masked?0x10:0))<<24)|((count&255)<<16)|0x8000|(zero?0x4000:0)|start;
            std::memcpy(packet.data(),&command,4);
            auto valueAt=[&](unsigned i){
                uint32_t raw=i*0x01030507u+0x80F08000u;
                if(vl==1)raw=zero?uint32_t(uint16_t(raw)):uint32_t(int32_t(int16_t(raw)));
                if(vl==2)raw=zero?uint32_t(uint8_t(raw)):uint32_t(int32_t(int8_t(raw)));
                return raw;
            };
            for(unsigned i=0;i<count*components;++i){const auto raw=valueAt(i);std::memcpy(packet.data()+4+i*bytes,&raw,bytes);}
            for(unsigned i=0;i<count;++i){
                unsigned dest=((start+29)+(i/wl)*cl+i%wl)&1023;
                for(unsigned lane=0;lane<4;++lane){
                    unsigned select=masked?lane:0;
                    uint32_t value=(vn==1)?valueAt(components*i+(lane%2)):(lane<components?valueAt(components*i+lane):expected[dest*4+lane]);
                    if(select==0){if(mode==1 || mode==2)value+=row[lane];if(mode==2 || mode==3)row[lane]=value;}
                    else if(select==1)value=row[lane];
                    else if(select==2)value=100+std::min(i%wl,3u);
                    else continue;
                    expected[dest*4+lane]=value;
                }
            }
            memory.processVIF1Data(reinterpret_cast<uint8_t*>(packet.data()),packet.size());
            if(std::memcmp(expected.data(),memory.getVU1Data(),sizeof(expected)) || std::memcmp(row.data(),memory.vif1_regs.row,16)){
                std::fprintf(stderr,"FAIL vn=%u vl=%u zero=%d cl=%u wl=%u start=%u count=%u mode=%u mask=%d\n",vn,vl,zero,cl,wl,start,count,mode,masked);return 2;
            }
            ++cases;
        }
    }
    // End each source exactly at an inaccessible page. A wider NEON load must
    // never consume padding past the last complete VIF vector.
    const size_t page=size_t(sysconf(_SC_PAGESIZE));
    auto *guard=static_cast<uint8_t *>(mmap(nullptr,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(guard==MAP_FAILED || mprotect(guard+page,page,PROT_NONE)!=0)return 3;
    memory.vif1_regs.mode=0;
    unsigned guarded=0;
    for(unsigned cl:{1u,4u,17u,255u})for(unsigned format:{0x65u,0x6au,0x6du,0x6eu})for(unsigned count=1;count<=17;++count)for(bool zero:{false,true}){
        memory.vif1_regs.cycle=0x0100|cl;
        const unsigned components=((format>>2)&3)+1,bytes=4u>>(format&3),size=4+((components*bytes*count+3u)&~3u);
        uint8_t *packet=guard+page-size;
        const uint32_t command=(format<<24)|(count<<16)|(zero?0x4000:0)|1023;
        std::memcpy(packet,&command,4);std::memset(packet+4,0x80,size-4);
        memory.processVIF1Data(packet,size);++guarded;
    }
    munmap(guard,page*2);
    std::printf("%u source-end guard-page cases: PASS\n",guarded);
    std::printf("%u V2/V3/V4-32/16/8 signed and unsigned cases: STCYCL skips, address wrap, TOPS, NUM=0, modes and mask fallback: PASS\n",cases);
    memory.vif1_regs.mode=0;
    for(unsigned cycle:{0x0101u,0x0104u})for(unsigned format:{0x65u,0x6au,0x6du,0x6eu}){
        memory.vif1_regs.cycle=cycle;
        const unsigned vn=(format>>2)&3,vl=format&3;
        std::vector<uint32_t> packet(1+256*(vn+1)/(1u<<vl),0x80ff017f);
        packet[0]=format<<24; // NUM=0, 256 vectors.
        const auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<50000;++i)memory.processVIF1Data(reinterpret_cast<uint8_t*>(packet.data()),packet.size()*4);
        std::printf("V%u-%u CL=%u WL=%u: 12800000 vectors in %.3f ms\n",vn+1,32u>>vl,cycle&255,cycle>>8,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    }
}
