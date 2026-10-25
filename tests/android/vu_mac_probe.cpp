// Raw-bit oracle for the MAC flags used by generated VU1 blocks.
#include "runtime/ps2_vu1_native.h"
#include "runtime/ps2_fp_control.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

template<int Mask>
__attribute__((noinline)) uint32_t nativeFlags(__m128 value)
{
    return vu1n::macVal<Mask>(value);
}

__attribute__((noinline)) uint32_t referenceFlags(__m128 value)
{
    return vu1n::macValRt(value, 7u);
}

uint32_t oracle(const std::array<uint32_t, 4> &bits, uint32_t mask)
{
    uint32_t flags = 0;
    for (unsigned lane = 0; lane < 4; ++lane)
    {
        if (!(mask & (1u << lane)))
            continue;
        const uint32_t magnitude = bits[lane] & 0x7FFFFFFFu;
        const uint32_t weight = 8u >> lane;
        if (magnitude == 0)
            flags |= weight;
        if (bits[lane] & 0x80000000u)
            flags |= weight << 4;
        if ((magnitude & 0x7F800000u) == 0x7F800000u || magnitude == 0x7F7FFFFFu)
            flags |= weight << 12;
    }
    return flags;
}

int main()
{
    const unsigned saved = ps2_fp::readControl();
    struct Restore { unsigned value; ~Restore() { ps2_fp::writeControl(value); } } restore{saved};
    constexpr uint32_t edge[] = {0, 0x80000000u, 1, 0x80000001u, 0x007FFFFFu,
        0x3F800000u, 0xBF800000u, 0x7F7FFFFEu, 0x7F7FFFFFu, 0xFF7FFFFFu,
        0x7F800000u, 0xFF800000u, 0x7FC00000u, 0xFFC00000u, 0x7F800001u, 0xFFFFFFFFu};
    std::vector<std::array<uint32_t, 4>> input(131072);
    std::mt19937 random(8319);
    for (unsigned i = 0; i < 65536; ++i)
        for (unsigned lane = 0; lane < 4; ++lane)
        {
            input[i][lane] = edge[(i >> (lane * 4)) & 15];
            input[i + 65536][lane] = random();
        }
    using FlagsFn = uint32_t (*)(__m128);
    const FlagsFn functions[] = {nativeFlags<0>, nativeFlags<1>, nativeFlags<2>, nativeFlags<3>,
        nativeFlags<4>, nativeFlags<5>, nativeFlags<6>, nativeFlags<7>, nativeFlags<8>, nativeFlags<9>,
        nativeFlags<10>, nativeFlags<11>, nativeFlags<12>, nativeFlags<13>, nativeFlags<14>, nativeFlags<15>};
    unsigned cases = 0;
    for (const unsigned csr : {saved, (saved & ~0x6000u) | 0x6000u | 0x8000u | 0x40u})
    {
        ps2_fp::writeControl(csr);
        for (const auto &bits : input)
            for (unsigned mask = 0; mask < 16; ++mask)
            {
                const __m128 value = _mm_loadu_ps(reinterpret_cast<const float *>(bits.data()));
                const uint32_t expected = oracle(bits, mask);
                if (functions[mask](value) != expected || vu1n::macValRt(value, mask) != expected)
                {
                    std::fprintf(stderr, "MAC mismatch mask=%u csr=%x\n", mask, csr);
                    return 1;
                }
                ++cases;
            }
    }
    std::printf("%u MAC/DEST cases: signed zero, subnormals, FLT_MAX, infinity, NaN and random raw bits; host/VU FP control: PASS\n", cases);
    for (unsigned round = 0; round < 3; ++round)
        for (const FlagsFn fn : {referenceFlags, nativeFlags<7>})
        {
            uint64_t sink = 0;
            const auto begin = std::chrono::steady_clock::now();
            for (unsigned repeat = 0; repeat < 64; ++repeat)
                for (const auto &bits : input)
                    sink += fn(_mm_loadu_ps(reinterpret_cast<const float *>(bits.data())));
            std::printf("%s: 8388608 MAC flags %.3f ms (sum %llu)\n",
                fn == referenceFlags ? "SSE reference" : "native",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count(),
                static_cast<unsigned long long>(sink));
        }
}
