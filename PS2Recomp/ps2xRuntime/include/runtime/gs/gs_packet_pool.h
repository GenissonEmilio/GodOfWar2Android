#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

// Owned GIF packet storage. Small packets must not inherit the capacity of an
// old IMAGE upload: bounding queued payload bytes alone does not bound that
// retained storage. Exact power-of-two classes keep capacity < 2 * payload
// (minimum 16 bytes), and cached buffers have independent byte/count limits.
class GSPacketPool
{
public:
    static constexpr size_t maxBuffers = 64;
    static constexpr size_t maxBytes = 8u << 20;
    static constexpr size_t maxBufferBytes = 1u << 20;
    static constexpr size_t minBufferBytes = 16;
    static constexpr size_t maxPerClass = 8;
private:
    std::array<std::vector<std::vector<uint8_t>>, 17> bins;
    size_t retainedBytes = 0, retainedBuffers = 0;
public:
    size_t bytes() const { return retainedBytes; }
    size_t count() const { return retainedBuffers; }
    std::vector<uint8_t> take(size_t size)
    {
        std::vector<uint8_t> buffer;
        if (size > maxBufferBytes)
        {
            buffer.reserve(size);
            return buffer;
        }
        const size_t capacity = std::bit_ceil(std::max(size, minBufferBytes));
        auto &bin = bins[std::countr_zero(capacity) - 4u];
        if (!bin.empty())
        {
            buffer = std::move(bin.back());
            bin.pop_back();
            retainedBytes -= buffer.capacity();
            --retainedBuffers;
        }
        else buffer.reserve(capacity);
        return buffer;
    }
    bool recycle(std::vector<uint8_t> &&buffer)
    {
        const size_t capacity = buffer.capacity();
        if (capacity < minBufferBytes || capacity > maxBufferBytes || !std::has_single_bit(capacity))
            return false;
        const unsigned index = std::countr_zero(capacity) - 4u;
        if (bins[index].size() == maxPerClass) return false;
        // A burst of tiny packets must leave room for the next IMAGE upload,
        // and a cache of IMAGE buffers must leave room for small VU packets.
        while (retainedBuffers == maxBuffers || retainedBytes + capacity > maxBytes)
        {
            size_t victim = bins.size();
            for (size_t i = bins.size(); i-- > 0;)
                if (i != index && !bins[i].empty()) { victim = i; break; }
            if (victim == bins.size()) return false;
            retainedBytes -= bins[victim].back().capacity();
            bins[victim].pop_back();
            --retainedBuffers;
        }
        buffer.clear();
        bins[index].push_back(std::move(buffer));
        retainedBytes += capacity;
        ++retainedBuffers;
        return true;
    }
};
