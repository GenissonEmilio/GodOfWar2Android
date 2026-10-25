#pragma once
#include <android/performance_hint.h>
#include "runtime/ps2_worker_ids.h"
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <dlfcn.h>
#include <vector>

// NDK hints are optional (API 33+). Resolve at runtime so API 29 remains
// supported. Only the game executor owns and calls its session.
class AndroidPerformanceHint
{
    struct Api
    {
        APerformanceHintManager *(*manager)() = nullptr;
        APerformanceHintSession *(*create)(APerformanceHintManager*,const int32_t*,size_t,int64_t) = nullptr;
        void (*close)(APerformanceHintSession*) = nullptr;
        int (*report)(APerformanceHintSession*,int64_t) = nullptr;
        int64_t (*rate)(APerformanceHintManager*) = nullptr;
        Api()
        {
#define GOW_HINT_LOAD(member, name) member = reinterpret_cast<decltype(member)>(dlsym(RTLD_DEFAULT, name))
            GOW_HINT_LOAD(manager,"APerformanceHint_getManager");
            GOW_HINT_LOAD(create,"APerformanceHint_createSession");
            GOW_HINT_LOAD(close,"APerformanceHint_closeSession");
            GOW_HINT_LOAD(report,"APerformanceHint_reportActualWorkDuration");
            GOW_HINT_LOAD(rate,"APerformanceHint_getPreferredUpdateRateNanos");
#undef GOW_HINT_LOAD
        }
        bool available() const { return manager && create && close && report && rate; }
    };
    static const Api &api() { static const Api instance; return instance; }
    APerformanceHintSession *session = nullptr;
    uint64_t seenEpoch = ~0ull, lastReport = 0;
    int seenTarget = 0;
    int64_t updateRate = 0;

    void open(int target)
    {
        const auto &a=api();
        if(!a.available())return;
        auto *manager=a.manager();
        if(!manager || (updateRate=a.rate(manager))<=0)return;
        std::vector<int32_t> tids;
        for(const auto &worker : ps2_workers::ids)
        {
            const int32_t tid=worker.load(std::memory_order_acquire);
            if(tid>0 && std::find(tids.begin(),tids.end(),tid)==tids.end())tids.push_back(tid);
        }
        if(tids.empty())return;
        session=a.create(manager,tids.data(),tids.size(),1000000000ll/target);
        std::fprintf(stderr,"[gow-adpf] session=%s threads=%zu target_ns=%lld update_ns=%lld",
            session?"active":"unsupported",tids.size(),1000000000ll/target,static_cast<long long>(updateRate));
        for(const auto tid:tids)std::fprintf(stderr," tid=%d",tid);
        std::fputc('\n',stderr);
    }
public:
    AndroidPerformanceHint()=default;
    AndroidPerformanceHint(const AndroidPerformanceHint&)=delete;
    ~AndroidPerformanceHint(){if(session)api().close(session);}
    void sample(uint64_t now,uint64_t workStart,int target,uint64_t epoch,bool enabled)
    {
        if(epoch!=seenEpoch || target!=seenTarget)
        {
            if(session){api().close(session);session=nullptr;}
            seenEpoch=epoch;seenTarget=target;lastReport=0;
            if(enabled)open(target);
        }
        if(session && workStart && now>workStart && now-lastReport>=uint64_t(updateRate))
        {
            // Excludes the deliberate frame-limiter sleep. This is the measured
            // critical path, including real dependencies on the other workers.
            const int error=api().report(session,static_cast<int64_t>(now-workStart));lastReport=now;
            if(error){std::fprintf(stderr,"[gow-adpf] reporting disabled: error=%d\n",error);api().close(session);session=nullptr;}
        }
    }
};
