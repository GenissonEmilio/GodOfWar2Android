#pragma once
#include <algorithm>
#include <cstdint>

class GameFrameClock
{
public:
    static constexpr int target(int requested)
    {
        return requested == 24 || requested == 60 ? requested : 30;
    }
    static constexpr int presentationTarget(int requested, bool movie)
    {
        const int limit = target(requested);
        // The optional combat budget must not slow the 29.97 FPS video clock.
        return movie && limit == 24 ? 30 : limit;
    }
    uint64_t deadline(uint64_t now, int target, uint64_t epoch)
    {
        target = GameFrameClock::target(target);
        if (epoch != m_epoch || target != m_target)
        {
            m_next = now;
            m_epoch = epoch;
            m_target = target;
        }
        if (target == 60)
            return now;
        const uint64_t at = std::max(now, m_next);
        m_next = at + 1000000000ull / static_cast<unsigned>(target);
        return at;
    }
private:
    uint64_t m_next = 0, m_epoch = 0;
    int m_target = 0;
};
