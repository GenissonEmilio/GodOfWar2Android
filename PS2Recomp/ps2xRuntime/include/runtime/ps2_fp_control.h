#pragma once
#include <cstdint>
#if defined(USE_SSE2NEON)
#include <sse2neon.h>
#else
#include <immintrin.h>
#endif

namespace ps2_fp
{
    // Same supported fields as SSE2NEON's MXCSR adapter, from one FPCR read.
    // ARM orders upward/downward rounding oppositely to MXCSR; FZ and DAZ
    // both map to FPCR.FZ. Preserve every unrelated FPCR bit on writes.
    inline uint32_t readControl()
    {
#if defined(USE_SSE2NEON) && defined(__aarch64__)
        uint64_t fpcr;
        asm volatile("mrs %0, fpcr" : "=r"(fpcr));
        return uint32_t(((fpcr >> 8u) & 0x4000u) | ((fpcr >> 10u) & 0x2000u) |
                        ((fpcr & (1ull << 24u)) ? 0x8040u : 0u));
#else
        return _mm_getcsr();
#endif
    }
    inline void writeControl(uint32_t csr)
    {
#if defined(USE_SSE2NEON) && defined(__aarch64__)
        uint64_t fpcr;
        asm volatile("mrs %0, fpcr" : "=r"(fpcr));
        const uint64_t value=(fpcr & ~(7ull << 22u)) | (uint64_t(csr & 0x4000u) << 8u) |
            (uint64_t(csr & 0x2000u) << 10u) | ((csr & 0x8040u) ? 1ull << 24u : 0u);
        asm volatile("msr fpcr, %0" :: "r"(value));
#else
        _mm_setcsr(csr);
#endif
    }
}
