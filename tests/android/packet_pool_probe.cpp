#include "runtime/gs/gs_packet_pool.h"
#include <cstdio>
#include <cstring>

int main()
{
    GSPacketPool pool;
    // Reproduce an IMAGE upload followed by a burst of tiny VU packets. Cached
    // storage must remain bounded and never inflate the tiny packets' capacity.
    auto image = pool.take(262144);
    image.resize(262144, 0x91);
    const auto *storage = image.data();
    if (!pool.recycle(std::move(image))) return 1;
    std::vector<std::vector<uint8_t>> burst;
    for (unsigned i = 0; i < 4096; ++i)
    {
        auto packet = pool.take(16);
        if (packet.capacity() != 16) return 2;
        packet.resize(16, uint8_t(i));
        burst.push_back(std::move(packet));
    }
    for (unsigned i = 0; i < burst.size(); ++i)
        for (uint8_t byte : burst[i]) if (byte != uint8_t(i)) return 3;
    for (auto &packet : burst) pool.recycle(std::move(packet));
    if (pool.count() != GSPacketPool::maxPerClass + 1 || pool.bytes() > GSPacketPool::maxBytes) return 4;
    auto reused = pool.take(262144);
    if (reused.data() != storage || !reused.empty()) return 5;
    // Large transfers remain supported but are never retained in the cache.
    auto huge = pool.take(GSPacketPool::maxBufferBytes + 1);
    huge.resize(GSPacketPool::maxBufferBytes + 1, 0x43);
    if (pool.recycle(std::move(huge))) return 6;
    GSPacketPool byteLimited;
    burst.clear();
    for (unsigned i = 0; i < 16; ++i) burst.push_back(byteLimited.take(GSPacketPool::maxBufferBytes));
    for (auto &packet : burst) byteLimited.recycle(std::move(packet));
    if (byteLimited.bytes() != GSPacketPool::maxBytes || byteLimited.count() != 8) return 7;
    auto tiny = byteLimited.take(16);
    if (!byteLimited.recycle(std::move(tiny)) || byteLimited.bytes() >= GSPacketPool::maxBytes) return 10;
    unsigned cases = 0;
    for (unsigned round = 0; round < 20; ++round)
        for (size_t size = 16; size <= GSPacketPool::maxBufferBytes; size = size * 2 + 1)
        {
            auto packet = pool.take(size);
            if (packet.capacity() < size || packet.capacity() >= size * 2) return 8;
            packet.assign(size, uint8_t(round));
            pool.recycle(std::move(packet));
            if (pool.count() > GSPacketPool::maxBuffers || pool.bytes() > GSPacketPool::maxBytes) return 9;
            ++cases;
        }
    std::printf("Packet pool: IMAGE/tiny burst, immutable owned bytes, reuse, oversized transfer, byte/count bounds and %u mixed-size cycles: PASS\n", cases);
}
