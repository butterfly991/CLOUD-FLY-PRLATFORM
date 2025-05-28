#include "core/optimization/simd_ops.h"
#if defined(__AVX2__)
#include <immintrin.h>
#endif
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#include <stddef.h>
#include <stdint.h>

void core_vector_add_f32(float* dst, const float* src1, const float* src2, size_t n) {
#if defined(__AVX2__)
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 a = _mm256_loadu_ps(&src1[i]);
        __m256 b = _mm256_loadu_ps(&src2[i]);
        __m256 c = _mm256_add_ps(a, b);
        _mm256_storeu_ps(&dst[i], c);
    }
    for (; i < n; ++i) dst[i] = src1[i] + src2[i];
#elif defined(__ARM_NEON)
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t a = vld1q_f32(&src1[i]);
        float32x4_t b = vld1q_f32(&src2[i]);
        float32x4_t c = vaddq_f32(a, b);
        vst1q_f32(&dst[i], c);
    }
    for (; i < n; ++i) dst[i] = src1[i] + src2[i];
#else
    for (size_t i = 0; i < n; ++i) dst[i] = src1[i] + src2[i];
#endif
}

void core_vector_add_i32(int32_t* dst, const int32_t* src1, const int32_t* src2, size_t n) {
#if defined(__AVX2__)
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256i a = _mm256_loadu_si256((__m256i*)&src1[i]);
        __m256i b = _mm256_loadu_si256((__m256i*)&src2[i]);
        __m256i c = _mm256_add_epi32(a, b);
        _mm256_storeu_si256((__m256i*)&dst[i], c);
    }
    for (; i < n; ++i) dst[i] = src1[i] + src2[i];
#elif defined(__ARM_NEON)
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        int32x4_t a = vld1q_s32(&src1[i]);
        int32x4_t b = vld1q_s32(&src2[i]);
        int32x4_t c = vaddq_s32(a, b);
        vst1q_s32(&dst[i], c);
    }
    for (; i < n; ++i) dst[i] = src1[i] + src2[i];
#else
    for (size_t i = 0; i < n; ++i) dst[i] = src1[i] + src2[i];
#endif
} 