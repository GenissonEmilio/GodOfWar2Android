#include "runtime/ps2_fp_control.h"
#include <chrono>
#include <cstdio>
static uint64_t readFpcr(){uint64_t value;asm volatile("mrs %0, fpcr":"=r"(value));return value;}
static void writeFpcr(uint64_t value){asm volatile("msr fpcr, %0"::"r"(value));}
int main(){
    const auto saved=readFpcr();
    struct Restore{uint64_t value;~Restore(){writeFpcr(value);}}restore{saved};
    unsigned cases=0;
    for(unsigned extra:{0u,1u<<19u,1u<<25u,(1u<<19u)|(1u<<25u)})
    for(unsigned mode=0;mode<4;++mode)for(unsigned flush=0;flush<4;++flush)for(unsigned noise:{0u,0x1fbfu}){
        const auto base=(saved & ~(uint64_t(7u)<<22u))|extra;
        const unsigned csr=(mode<<13u)|((flush&1)?0x8000:0)|((flush&2)?0x40:0)|noise;
        writeFpcr(base);_mm_setcsr(csr);const auto expected=readFpcr();const auto expectedCsr=_mm_getcsr();
        writeFpcr(base);ps2_fp::writeControl(csr);
        if(readFpcr()!=expected || ps2_fp::readControl()!=expectedCsr){std::fprintf(stderr,"FPCR mismatch mode=%u flush=%u\n",mode,flush);return 1;}
        ++cases;
    }
    std::printf("%u FPCR cases against SSE2NEON: all rounding/FZ/DAZ combinations, ignored fields and preserved control bits: PASS\n",cases);
    writeFpcr(saved);unsigned sink=0;
    for(bool direct:{false,true}){
        const auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<10000000;++i)sink^=direct?ps2_fp::readControl():_mm_getcsr();
        std::printf("%s readControl: 10000000 reads %.3f ms (sink %u)\n",direct?"single FPCR":"SSE2NEON",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),sink);
    }
}
