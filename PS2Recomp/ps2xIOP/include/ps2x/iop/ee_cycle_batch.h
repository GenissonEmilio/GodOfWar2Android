#pragma once
#include <cstdint>

namespace ps2x::iop
{
    // One IOP cycle per eight EE cycles. Keep the fractional remainder across
    // grants, and preserve the existing 512-IOP-cycle service boundary.
    class EeCycleBatch
    {
    public:
        bool accrue(uint64_t cycles) noexcept
        {
            m_pending += cycles;
            return m_pending >= 8u * 512u;
        }
        uint64_t takeIopCycles() noexcept
        {
            const uint64_t cycles = m_pending / 8u;
            m_pending %= 8u;
            return cycles;
        }
        void reset() noexcept { m_pending = 0u; }
    private:
        uint64_t m_pending = 0u;
    };
}
