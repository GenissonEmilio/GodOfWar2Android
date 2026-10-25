#pragma once
#include "runtime/ps2_worker_ids.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <sched.h>
#include <vector>

// Diagnostic A/B only. Android's scheduler and ADPF remain the default. Never
// expand a worker's allowed CPU set, change CPU frequencies or touch other apps.
class AndroidWorkerAffinity
{
    static constexpr unsigned count = static_cast<unsigned>(ps2_workers::Role::Count);
    std::array<int32_t, count> tids{};
    std::array<cpu_set_t, count> original{};
    int mode = 0;
    int requested = -1;

    void restore()
    {
        for (unsigned i = 0; i < count; ++i)
            if (tids[i] && ps2_workers::ids[i].load(std::memory_order_acquire) == tids[i])
                if (sched_setaffinity(tids[i], sizeof(cpu_set_t), &original[i]) != 0)
                    std::fprintf(stderr, "[gow-affinity] restore role=%u error=%d\n", i, errno);
        tids.fill(0);
        mode = 0;
    }
public:
    struct Cpu { int index; unsigned capacity; };
    static std::array<int, count> select(std::vector<Cpu> cpus)
    {
        std::array<int, count> result;
        result.fill(-1);
        std::sort(cpus.begin(), cpus.end(), [](const Cpu &a, const Cpu &b) {
            return a.capacity != b.capacity ? a.capacity > b.capacity : a.index > b.index;
        });
        // Four actual performance cores must be available. Unknown topology and
        // two-big-core devices retain normal scheduling.
        if (cpus.size() < count || !cpus[0].capacity || cpus[count - 1].capacity * 4u < cpus[0].capacity * 3u)
            return result;
        result[static_cast<unsigned>(ps2_workers::Role::Vu)] = cpus[0].index;
        result[static_cast<unsigned>(ps2_workers::Role::Ee)] = cpus[1].index;
        result[static_cast<unsigned>(ps2_workers::Role::Gs)] = cpus[2].index;
        result[static_cast<unsigned>(ps2_workers::Role::Frontend)] = cpus[3].index;
        return result;
    }
    AndroidWorkerAffinity() = default;
    AndroidWorkerAffinity(const AndroidWorkerAffinity &) = delete;
    ~AndroidWorkerAffinity() { if (mode) restore(); }
    bool setMode(int wanted)
    {
        if (requested == wanted) return false;
        requested = wanted;
        const bool wasActive = mode != 0;
        if (mode) restore();
        if (!wanted)
        {
            if (wasActive) std::fprintf(stderr, "[gow-affinity] mode=default restored\n");
            return wasActive;
        }
        cpu_set_t common;
        CPU_ZERO(&common);
        std::array<int32_t, count> current;
        for (unsigned i = 0; i < count; ++i)
        {
            current[i] = ps2_workers::ids[i].load(std::memory_order_acquire);
            if (current[i] <= 0) { requested = -1; return wasActive; }
            if (sched_getaffinity(current[i], sizeof(cpu_set_t), &original[i]) != 0)
            {
                std::fprintf(stderr, "[gow-affinity] read role=%u error=%d\n", i, errno);
                return wasActive;
            }
            if (!i) common = original[i];
            else CPU_AND(&common, &common, &original[i]);
        }
        std::vector<Cpu> cpus;
        for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
        {
            if (!CPU_ISSET(cpu, &common)) continue;
            char path[128];
            std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpu_capacity", cpu);
            unsigned capacity = 0;
            if (FILE *file = std::fopen(path, "r"))
            {
                if (std::fscanf(file, "%u", &capacity) != 1) capacity = 0;
                std::fclose(file);
            }
            cpus.push_back({cpu, capacity});
        }
        const auto chosen = select(cpus);
        if (chosen[0] < 0)
        {
            std::fprintf(stderr, "[gow-affinity] unavailable topology; default retained\n");
            return wasActive;
        }
        cpu_set_t pool;
        CPU_ZERO(&pool);
        for (int cpu : chosen) CPU_SET(cpu, &pool);
        for (unsigned i = 0; i < count; ++i)
        {
            cpu_set_t mask = pool;
            if (wanted == 1) { CPU_ZERO(&mask); CPU_SET(chosen[i], &mask); }
            if (sched_setaffinity(current[i], sizeof(cpu_set_t), &mask) != 0)
            {
                std::fprintf(stderr, "[gow-affinity] set role=%u error=%d; rollback\n", i, errno);
                restore();
                return wasActive;
            }
            tids[i] = current[i];
        }
        mode = wanted;
        std::fprintf(stderr, "[gow-affinity] mode=%s ee=%d vu=%d frontend=%d gs=%d\n",
            wanted == 1 ? "pin" : "pool", chosen[0], chosen[1], chosen[2], chosen[3]);
        return true;
    }
};
