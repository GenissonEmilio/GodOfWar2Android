#include "android_worker_affinity.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

int main()
{
    using Affinity = AndroidWorkerAffinity;
    const auto topology = Affinity::select({{0,418},{1,418},{2,418},{3,418},{4,907},{5,907},{6,907},{7,1024}});
    if (topology != std::array<int,4>{6,7,4,5}) return 1;
    if (Affinity::select({{0,400},{1,400},{4,1000},{5,1000}})[0] >= 0) return 2;
    if (Affinity::select({{0,0},{1,0},{2,0},{3,0}})[0] >= 0) return 3;
    if (Affinity::select({{4,900},{5,900},{6,900}})[0] >= 0) return 4;
    std::array<cpu_set_t,4> original;
    std::array<std::thread,4> threads;
    std::mutex mutex;
    std::condition_variable cv;
    bool stop = false;
    std::atomic<unsigned> ready{0};
    for (unsigned i = 0; i < 4; ++i)
        threads[i] = std::thread([&,i] {
            ps2_workers::Registration registration(static_cast<ps2_workers::Role>(i));
            sched_getaffinity(0, sizeof(cpu_set_t), &original[i]);
            ready.fetch_add(1);
            std::unique_lock lock(mutex);
            cv.wait(lock, [&] { return stop; });
        });
    while (ready.load() != 4) std::this_thread::yield();
    int error = 0;
    {
        Affinity affinity;
        const bool active = affinity.setMode(1);
        if (active)
        {
        for (unsigned i = 0; i < 4; ++i)
        {
            cpu_set_t actual, outside;
            if (sched_getaffinity(ps2_workers::ids[i].load(), sizeof(actual), &actual) != 0) error = 6;
            CPU_AND(&outside, &actual, &original[i]);
            if (CPU_COUNT(&actual) != 1 || !CPU_EQUAL(&actual, &outside)) error = 7;
        }
        if (!affinity.setMode(2)) error = 8;
        for (unsigned i = 0; i < 4; ++i)
        {
            cpu_set_t actual;
            sched_getaffinity(ps2_workers::ids[i].load(), sizeof(actual), &actual);
            if (CPU_COUNT(&actual) != 4) error = 9;
        }
        if (!affinity.setMode(0)) error = 10;
        if (affinity.setMode(0)) error = 11;
        }
        else std::puts("Runtime affinity unavailable; checking that default masks were retained");
        for (unsigned i = 0; i < 4; ++i)
        {
            cpu_set_t actual;
            sched_getaffinity(ps2_workers::ids[i].load(), sizeof(actual), &actual);
            if (!CPU_EQUAL(&actual, &original[i])) error = 12;
        }
    }
    { std::lock_guard lock(mutex); stop = true; }
    cv.notify_all();
    for (auto &thread : threads) thread.join();
    if (error) return std::fprintf(stderr, "Worker affinity probe failed: %d\n", error), error;
    std::puts("Topology, four own workers, pin/pool, original masks restored: PASS");
}
