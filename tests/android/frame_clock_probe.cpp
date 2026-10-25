#include "game_frame_clock.h"
#include <cstdio>
int main(){
    for (int target : {24,30}) {
        GameFrameClock clock;
        const uint64_t period=1000000000ull/target;
        uint64_t now=1000000000ull;
        for(unsigned frame=0;frame<100000;++frame){
            const auto expected=1000000000ull+frame*period;
            const auto deadline=clock.deadline(now,target,1);
            if(deadline!=expected)return 1;
            now=deadline+10000000; // Ten milliseconds of work; remaining time is paced.
        }
        now+=8000000000ull;
        if(clock.deadline(now,target,1)!=now || clock.deadline(now+10000000,target,1)!=now+period)return 2;
        // Pause/resume and target changes discard timing debt.
        if(clock.deadline(now+11000000,target,2)!=now+11000000)return 3;
        if(clock.deadline(now+12000000,60,2)!=now+12000000)return 4;
        if(clock.deadline(now+13000000,target,2)!=now+13000000)return 5;
        const int next=target==24?30:24;
        if(clock.deadline(now+14000000,next,2)!=now+14000000)return 6;
        if(clock.deadline(now+15000000,next,2)!=now+14000000+1000000000ull/next)return 7;
    }
    for(int invalid : {-1,0,1,25,120})if(GameFrameClock::target(invalid)!=30)return 8;
    for(int target : {24,30,60}){
        if(GameFrameClock::presentationTarget(target,false)!=target)return 9;
        if(GameFrameClock::presentationTarget(target,true)!=(target==24?30:target))return 10;
    }
    std::puts("Game frame pacing: 200,000 deadlines at 24/30 FPS, slow frames, pause/target changes and independent video presentation: PASS");
}
