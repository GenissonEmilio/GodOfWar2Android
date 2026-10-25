// Independent raw-bit normalization and scalar CLIP oracle for the native VU.
#include "runtime/ps2_vu1_native.h"
#include "runtime/ps2_fp_control.h"
#include <array>
#include <bit>
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

using Bits = std::array<uint32_t, 4>;
uint32_t normalized(uint32_t bits)
{
    return (bits & 0x7F800000u) == 0x7F800000u ? (bits & 0x80000000u) | 0x7F7FFFFFu : bits;
}
__attribute__((noinline)) __m128 nativeNormalize(__m128 value) { return vu1n::nrm(value); }
__attribute__((noinline)) __m128 referenceNormalize(__m128 value)
{
    const __m128i bits = _mm_castps_si128(value);
    const __m128i exponent = _mm_set1_epi32(0x7F800000);
    const __m128i bad = _mm_cmpeq_epi32(_mm_and_si128(bits, exponent), exponent);
    const __m128i fixed = _mm_or_si128(_mm_and_si128(bits, _mm_set1_epi32(int(0x80000000u))), _mm_set1_epi32(0x7F7FFFFF));
    return _mm_castsi128_ps(_mm_blendv_epi8(bits, fixed, bad));
}
__attribute__((noinline)) uint32_t scalarClip(__m128 value, __m128 threshold, uint32_t previous)
{
    alignas(16) float v[4], t[4];
    _mm_store_ps(v, value); _mm_store_ps(t, threshold);
    const float w = std::fabs(t[3]);
    uint32_t flags = 0;
    if (v[0] > w) flags |= 1u;
    if (v[0] < -w) flags |= 2u;
    if (v[1] > w) flags |= 4u;
    if (v[1] < -w) flags |= 8u;
    if (v[2] > w) flags |= 16u;
    if (v[2] < -w) flags |= 32u;
    return ((previous << 6) | flags) & 0xFFFFFFu;
}
__attribute__((noinline)) uint32_t nativeClip(__m128 value, __m128 threshold, uint32_t previous)
{
    return vu1n::clipValv(value, threshold, previous);
}
int main()
{
    const unsigned saved = ps2_fp::readControl();
    struct Restore { unsigned value; ~Restore() { ps2_fp::writeControl(value); } } restore{saved};
    constexpr uint32_t edge[] = {0, 0x80000000u, 1, 0x80000001u, 0x007FFFFFu,
        0x00800000u, 0x3F800000u, 0xBF800000u, 0x7F7FFFFEu, 0x7F7FFFFFu,
        0xFF7FFFFFu, 0x7F800000u, 0xFF800000u, 0x7FC00000u, 0x7F800001u, 0xFFFFFFFFu};
    std::mt19937 random(50412);
    std::vector<Bits> input(131072);
    for (unsigned i = 0; i < input.size(); ++i)
        for (unsigned lane = 0; lane < 4; ++lane)
            input[i][lane] = i < 65536 ? edge[(i >> (lane * 4)) & 15] : random();
    unsigned normalizationCases = 0, clipCases = 0;
    for (const unsigned csr : {saved, (saved & ~0x6000u) | 0x6000u | 0x8000u | 0x40u})
    {
        ps2_fp::writeControl(csr);
        for (const auto &bits : input)
        {
            const __m128 value = _mm_loadu_ps(reinterpret_cast<const float *>(bits.data()));
            Bits actual;
            _mm_storeu_ps(reinterpret_cast<float *>(actual.data()), nativeNormalize(value));
            for (unsigned lane = 0; lane < 4; ++lane)
            {
                if (actual[lane] != normalized(bits[lane])) return std::fprintf(stderr, "Normalization mismatch csr=%x\n", csr), 1;
                ++normalizationCases;
            }
            for (const uint32_t w : edge)
            {
                Bits threshold{uint32_t(random()), uint32_t(random()), uint32_t(random()), w};
                const __m128 t = _mm_loadu_ps(reinterpret_cast<const float *>(threshold.data()));
                const uint32_t previous = random();
                if (nativeClip(value, t, previous) != scalarClip(value, t, previous))
                    return std::fprintf(stderr, "Raw CLIP mismatch csr=%x\n", csr), 1;
                VU1State state{};
                _mm_storeu_ps(state.vf[3], value); _mm_storeu_ps(state.vf[6], t);
                Bits nv, nt;
                for (unsigned lane = 0; lane < 4; ++lane) nv[lane] = normalized(bits[lane]), nt[lane] = normalized(threshold[lane]);
                if (vu1n::clipVal(state, 3, 6, previous) != scalarClip(
                    _mm_loadu_ps(reinterpret_cast<const float *>(nv.data())),
                    _mm_loadu_ps(reinterpret_cast<const float *>(nt.data())), previous))
                    return std::fprintf(stderr, "Register CLIP mismatch csr=%x\n", csr), 1;
                clipCases += 2;
            }
        }
    }
    std::printf("%u normalization lanes and %u CLIP cases (raw/register, 24-bit history, host/VU FP): PASS\n", normalizationCases, clipCases);
    ps2_fp::writeControl((saved & ~0x6000u) | 0x6000u | 0x8000u | 0x40u);
    for (unsigned round = 0; round < 3; ++round)
    {
        for (const auto fn : {referenceNormalize, nativeNormalize})
        {
            uint64_t sum = 0;
            const auto begin = std::chrono::steady_clock::now();
            for (unsigned repeat = 0; repeat < 64; ++repeat)
                for (const auto &bits : input)
                    sum += uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(fn(_mm_loadu_ps(reinterpret_cast<const float *>(bits.data()))))));
            std::printf("%s normalize 8388608 vectors: %.3f ms sum=%llu\n", fn == referenceNormalize ? "reference" : "native",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count(), (unsigned long long)sum);
        }
        for (const auto fn : {scalarClip, nativeClip})
        {
            uint64_t sum = 0;
            const auto begin = std::chrono::steady_clock::now();
            for (unsigned repeat = 0; repeat < 64; ++repeat)
                for (const auto &bits : input)
                    sum += fn(_mm_loadu_ps(reinterpret_cast<const float *>(bits.data())), _mm_set1_ps(1.0f), repeat);
            std::printf("%s CLIP 8388608 vectors: %.3f ms sum=%llu\n", fn == scalarClip ? "reference" : "native",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count(), (unsigned long long)sum);
        }
    }
}
