#include "game_frame_clock.h"
#include <cstdio>
int main(){
    GameFrameClock clock;
    constexpr uint64_t period=1000000000ull/30;
    uint64_t now=1000000000ull;
    for(unsigned frame=0;frame<100000;++frame){
        const auto expected=1000000000ull+frame*period;
        const auto deadline=clock.deadline(now,30,1);
        if(deadline!=expected)return 1;
        now=deadline+10000000; // Ten milliseconds of work; remaining time is paced.
    }
    now+=8000000000ull;
    if(clock.deadline(now,30,1)!=now || clock.deadline(now+10000000,30,1)!=now+period)return 2;
    // Pause/resume and target changes discard timing debt.
    if(clock.deadline(now+11000000,30,2)!=now+11000000)return 3;
    if(clock.deadline(now+12000000,60,2)!=now+12000000)return 4;
    if(clock.deadline(now+13000000,30,2)!=now+13000000)return 5;
    std::puts("Game frame pacing: 100,000 deadlines, slow frames, pause epoch and target changes: PASS");
}
