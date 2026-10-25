#include "ps2x/iop/ee_cycle_batch.h"
#include <cstdint>
#include <cstdio>
int main()
{
    ps2x::iop::EeCycleBatch clock;
    uint64_t carry=0, pending=0, granted=0, total=0;
    uint32_t random=19;
    for(unsigned i=0;i<1000000;++i)
    {
        random=random*1664525u+1013904223u;
        const uint64_t ee=(i%17==0)?(random&65535u):(random&31u);
        total+=ee;
        const uint64_t sum=carry+ee;carry=sum%8;pending+=sum/8;
        const bool due=clock.accrue(ee);
        if(due!=(pending>=512))return 1;
        // Include explicit partial drains, not just full batches.
        if(due || i%137==0){const auto actual=clock.takeIopCycles();if(actual!=pending)return 2;granted+=actual;pending=0;}
    }
    granted+=clock.takeIopCycles();
    if(granted!=total/8)return 3;
    (void)clock.accrue(4095);clock.reset();
    if(clock.takeIopCycles()!=0 || clock.accrue(4095) || !clock.accrue(1) || clock.takeIopCycles()!=512)return 4;
    std::puts("IOP cycle grants: one million partitions, fractional carry, service boundaries, partial drain and reset: PASS");
}
