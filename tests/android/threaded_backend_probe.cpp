#include "runtime/gs/gs_threaded_backend.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <vector>
static std::atomic<bool> asyncMode{true};
bool gowAndroidAsyncFinish(){return asyncMode.load();}
struct RecordingBackend final:GSRasterBackend {
    std::vector<int> operations;int last=0;std::atomic<unsigned> finished{0};
    void Initialize(uint8_t*,uint32_t)override{}void Reset()override{}
    void Submit(const GSPrimitiveBatch &b)override {
        last=int(b.vertices[0].x);
        if(b.state.context.alpha!=uint64_t(last%7))std::abort();
        operations.push_back(last);
    }
    void LoadClut(const GSTex0Reg&,const GSTexClutReg&)override{}
    void BeginTransfer(const GSTransferCommand&)override{}void UploadImage(const uint8_t*,uint32_t)override{}
    void Flush()override{}void TextureFlush()override{}
    void Sync(GSSyncReason)override{operations.push_back(-last);finished.fetch_add(1);}
    PresentationFrame Present(const GSPresentationRequest&)override{return{};}
    bool ClearFramebuffer(const GSContext&,uint32_t)override{return false;}
    uint32_t ConsumeLocalToHostBytes(uint8_t*,uint32_t)override{return 0;}
    uint32_t ReadVram(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t)const override{return operations.size();}
    void WriteVram(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t)override{}
    void SnapshotVram(std::vector<uint8_t>&)const override{}
    GSTransferSnapshot GetTransferSnapshot()const override{return{};}
};
int main(){
    auto inner=std::make_unique<RecordingBackend>();auto *record=inner.get();
    GSThreadedBackend queue(std::move(inner));queue.Initialize(nullptr,0);
    for(int i=1;i<=200;++i){GSPrimitiveBatch b{};b.vertexCount=1;b.vertices[0].x=i;b.state.context.alpha=i%7;queue.Submit(b);queue.Sync(GSSyncReason::Finish);}
    if(queue.ReadVram(0,0,0,0,0)!=400)return 1;
    for(int i=1;i<=200;++i)if(record->operations[2*i-2]!=i || record->operations[2*i-1]!=-i)return 2;
    asyncMode.store(false);queue.Sync(GSSyncReason::Finish);
    if(record->finished.load()!=201)return 3;
    auto submit=[&](int id){GSPrimitiveBatch b{};b.vertexCount=1;b.vertices[0].x=id;b.state.context.alpha=id%7;queue.Submit(b);};
    std::thread competing;
    std::atomic<bool> started{false};
    queue.CommandStream([&]{
        competing=std::thread([&]{started.store(true);submit(9001);});
        while(!started.load())std::this_thread::yield();
        for(int i=1000;i<6000;++i){
            submit(i);
            if(i==3000)queue.CommandStream([&]{queue.Sync(GSSyncReason::Finish);});
        }
        // A synchronous worker call must also complete while we own the stream.
        queue.ReadVram(0,0,0,0,0);
    });
    competing.join();queue.ReadVram(0,0,0,0,0);
    size_t at=401;
    for(int i=1000;i<6000;++i){if(record->operations[at++]!=i)return 4;if(i==3000 && record->operations[at++]!=-i)return 5;}
    if(record->operations[at++]!=9001 || at!=record->operations.size())return 6;
    try {queue.CommandStream([&]{throw std::runtime_error("scope test");});}catch(const std::runtime_error&){}
    submit(9002);queue.ReadVram(0,0,0,0,0);
    if(record->operations.back()!=9002)return 7;
    std::puts("Draw/FINISH order, readback, chunk backpressure, nested streams, competing producer and exception cleanup: PASS");
}
