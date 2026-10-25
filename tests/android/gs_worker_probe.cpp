// VU-owned MMIO can proceed behind copied frontend output. GS access cannot.
#include "runtime/ps2_memory.h"
#include "runtime/gs/gs_stage.h"
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <mutex>
bool gowAndroidAsyncFinish(){return false;}
int main(){
    setenv("PS2X_GS_WORKER","1",1);
    PS2Memory memory;if(!memory.initialize())return 1;
    std::mutex mutex;std::condition_variable cv;bool entered=false,release=false,bytesCorrect=true;
    std::array<uint8_t,16> source{};
    ps2_gs_stage::start([&](GifPathId,const uint8_t *data,uint32_t size){
        std::unique_lock lock(mutex);entered=true;cv.notify_all();cv.wait(lock,[&]{return release;});
        bytesCorrect &= size==16 && data[0]==42;
    },[]{});
    bool passed=true;
    for(uint32_t address:{0x10003C00u,0x11008000u,0x1100C000u,0x12001000u,0x10003020u}){
        {std::lock_guard lock(mutex);entered=false;release=false;}
        source[0]=42;ps2_gs_stage::push(GifPathId::Path1,source.data(),source.size());source[0]=99;
        {std::unique_lock lock(mutex);if(!cv.wait_for(lock,std::chrono::seconds(1),[&]{return entered;}))std::abort();}
        std::promise<void> started;auto start=started.get_future();
        auto read=std::async(std::launch::async,[&]{started.set_value();return memory.read32(address);});
        start.wait();
        const bool vuOwned=address==0x10003C00u || address==0x11008000u || address==0x1100C000u;
        const auto status=read.wait_for(vuOwned?std::chrono::milliseconds(500):std::chrono::milliseconds(50));
        if((status==std::future_status::ready)!=vuOwned){std::fprintf(stderr,"Barrier mismatch at %08x\n",address);passed=false;}
        {std::lock_guard lock(mutex);release=true;}cv.notify_all();
        if(read.wait_for(std::chrono::seconds(1))!=std::future_status::ready)std::abort();
        (void)read.get();ps2_gs_stage::waitIdle();
    }
    memory.stopGsWorker();
    if(!passed || !bytesCorrect)return 2;
    std::puts("VU/VIF ownership, immutable frontend packets and retained GS/GIF barriers: PASS");
}
