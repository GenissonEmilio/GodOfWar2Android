// GOW-Port: adaptado de Taylor N. Albarnaz / LightVelox (GPL-3.0).
// Origen: PS2Recomp sotc-port, ac9efa070638ad3b3accd284de6f898d5ab271d1.
#pragma once

#include <cstdlib>
#if defined(__ANDROID__)
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#ifndef WINAPI
#define WINAPI __stdcall
#endif

extern "C"
{
    __declspec(dllimport) void *WINAPI GetCurrentThread(void);
    __declspec(dllimport) int WINAPI SetThreadPriority(void *hThread, int nPriority);
}
#endif

namespace ThreadPriority
{
    enum class Level
    {
        AboveNormal = 1,
        Highest = 2
    };

    inline bool Enabled()
    {
        static const bool enabled = []
        {
            const char *value = std::getenv("PS2X_THREAD_PRIORITY");
            return !value || !*value || *value != '0';
        }();
        return enabled;
    }

    inline void RaiseCurrentThread(Level level)
    {
        if (!Enabled())
            return;
#if defined(_WIN32)
        ::SetThreadPriority(::GetCurrentThread(), static_cast<int>(level));
#elif defined(__ANDROID__)
        ::setpriority(PRIO_PROCESS, static_cast<id_t>(::syscall(SYS_gettid)), level==Level::Highest?-8:-4);
#else
        (void)level;
#endif
    }
}
