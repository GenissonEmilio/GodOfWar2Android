#include "runtime/gs/ps2_gif_arbiter.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
static void require(bool ok){if(!ok){std::fprintf(stderr,"GIF arbitration mismatch\n");std::exit(1);}}
int main(){
    using P=GifPathId;
    GifArbiter arbiter;
    std::array<uint64_t,4> image{(2ull<<58)|1,0,0,0};
    std::array<uint64_t,2> tag{0,0};
    std::vector<P> order;
    const uint8_t *borrowed=nullptr;
    arbiter.setProcessPathPacketFn([&](P path,const uint8_t *data,uint32_t){order.push_back(path);borrowed=data;});
    arbiter.submitAndDrain(P::Path1,reinterpret_cast<uint8_t*>(tag.data()),16);
    require(arbiter.empty() && borrowed==reinterpret_cast<uint8_t*>(tag.data()) && order==std::vector<P>{P::Path1});
    order.clear();
    arbiter.submit(P::Path3,reinterpret_cast<uint8_t*>(image.data()),32);
    arbiter.submit(P::Path2,reinterpret_cast<uint8_t*>(tag.data()),16,true);
    arbiter.submitAndDrain(P::Path1,reinterpret_cast<uint8_t*>(tag.data()),16);
    require(arbiter.empty() && order==std::vector<P>{P::Path1,P::Path3,P::Path2});
    // A directly processed IMAGE header must still classify its next fragment
    // as IMAGE when DIRECTHL is waiting in the queue.
    arbiter.reset();order.clear();
    arbiter.submitAndDrain(P::Path3,reinterpret_cast<uint8_t*>(image.data()),16);
    arbiter.submit(P::Path2,reinterpret_cast<uint8_t*>(tag.data()),16,true);
    arbiter.submitAndDrain(P::Path3,reinterpret_cast<uint8_t*>(image.data()+2),16);
    require(order==std::vector<P>{P::Path3,P::Path3,P::Path2});
    // Deferred payloads are owned; modifying the source cannot alter them.
    arbiter.reset();order.clear();tag[1]=91;
    arbiter.submit(P::Path2,reinterpret_cast<uint8_t*>(tag.data()),16);
    tag[1]=0;
    arbiter.setProcessPathPacketFn([&](P,const uint8_t *data,uint32_t){uint64_t value;std::memcpy(&value,data+8,8);require(value==91);});
    arbiter.drain();
    // Callback reentrancy is queued behind the packet already being processed.
    arbiter.setProcessPathPacketFn([&](P path,const uint8_t*,uint32_t){
        order.push_back(path);
        if(path==P::Path1){arbiter.submitAndDrain(P::Path2,reinterpret_cast<uint8_t*>(tag.data()),16);arbiter.drain();order.push_back(P::Path1);}
    });
    arbiter.submitAndDrain(P::Path1,reinterpret_cast<uint8_t*>(tag.data()),16);
    require(order==std::vector<P>{P::Path1,P::Path1,P::Path2} && arbiter.empty());
    // A failed callback releases the guard, so the next packet can be consumed.
    arbiter.setProcessPathPacketFn([](P,const uint8_t*,uint32_t){throw std::runtime_error("probe");});
    try{arbiter.submitAndDrain(P::Path1,reinterpret_cast<uint8_t*>(tag.data()),16);return 2;}catch(const std::runtime_error&){}
    unsigned calls=0;
    arbiter.setProcessPathPacketFn([&](P,const uint8_t*,uint32_t){++calls;});
    arbiter.submitAndDrain(P::Path1,reinterpret_cast<uint8_t*>(tag.data()),16);require(calls==1);
    std::puts("GIF arbiter: direct lifetime, deferred ownership, priorities, split IMAGE, reentrancy and exception cleanup: PASS");
    std::array<uint8_t,4096> payload{};uint64_t sink=0;
    arbiter.setProcessPathPacketFn([&](P,const uint8_t *data,uint32_t size){sink+=data[size-1];});
    for(bool direct:{false,true}){
        const auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<100000;++i){if(direct)arbiter.submitAndDrain(P::Path1,payload.data(),payload.size());else{arbiter.submit(P::Path1,payload.data(),payload.size());arbiter.drain();}}
        std::printf("%s: 100000 x 4096 bytes %.3f ms (sink %llu)\n",direct?"direct":"queued",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),static_cast<unsigned long long>(sink));
    }
}
