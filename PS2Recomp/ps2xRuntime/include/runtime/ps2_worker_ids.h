#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#if defined(__ANDROID__)
#include <unistd.h>
#endif

namespace ps2_workers
{
    enum class Role : unsigned { Ee, Vu, Frontend, Gs, Count };
    inline std::array<std::atomic<int32_t>, static_cast<unsigned>(Role::Count)> ids{};

    // Register the workers themselves: graphics drivers can inherit a worker's
    // name when spawning helpers, so /proc thread names cannot identify this set.
    class Registration
    {
        Role role;
        int32_t tid = 0;
    public:
        explicit Registration(Role value) : role(value)
        {
#if defined(__ANDROID__)
            tid = gettid();
            ids[static_cast<unsigned>(role)].store(tid, std::memory_order_release);
#endif
        }
        Registration(const Registration &) = delete;
        ~Registration()
        {
            if (tid)
                ids[static_cast<unsigned>(role)].compare_exchange_strong(tid, 0, std::memory_order_acq_rel);
        }
    };
}
