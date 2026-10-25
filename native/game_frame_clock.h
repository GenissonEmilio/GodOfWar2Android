#pragma once
#include <algorithm>
#include <cstdint>

class GameFrameClock
{
public:
    uint64_t deadline(uint64_t now, int target, uint64_t epoch)
    {
        if (epoch != m_epoch || target != m_target)
        {
            m_next = now;
            m_epoch = epoch;
            m_target = target;
        }
        if (target != 30)
            return now;
        const uint64_t at = std::max(now, m_next);
        m_next = at + 1000000000ull / 30u;
        return at;
    }
private:
    uint64_t m_next = 0, m_epoch = 0;
    int m_target = 0;
};
