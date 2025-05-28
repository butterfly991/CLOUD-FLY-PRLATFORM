#include "core/drivers/math_ops.h"
#include <math.h>

#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

// Быстрые математические функции с использованием таблиц и аппроксимаций
static const float sin_table[256] = {
    // Таблица предварительно вычисленных значений sin
    #include "sin_table.h"
};

static const float cos_table[256] = {
    // Таблица предварительно вычисленных значений cos
    #include "cos_table.h"
};

float core_fast_sin(float x) {
    // Нормализация угла в диапазон [0, 2π]
    x = fmodf(x, 2.0f * M_PI);
    if (x < 0) x += 2.0f * M_PI;
    
    // Преобразование в индекс таблицы
    int idx = (int)(x * 256.0f / (2.0f * M_PI)) & 0xFF;
    return sin_table[idx];
}

float core_fast_cos(float x) {
    // Нормализация угла в диапазон [0, 2π]
    x = fmodf(x, 2.0f * M_PI);
    if (x < 0) x += 2.0f * M_PI;
    
    // Преобразование в индекс таблицы
    int idx = (int)(x * 256.0f / (2.0f * M_PI)) & 0xFF;
    return cos_table[idx];
}

float core_fast_sqrt(float x) {
#if defined(__AVX2__)
    __m128 val = _mm_set_ss(x);
    val = _mm_sqrt_ss(val);
    return _mm_cvtss_f32(val);
#elif defined(__ARM_NEON)
    float32x2_t val = vdup_n_f32(x);
    val = vsqrt_f32(val);
    return vget_lane_f32(val, 0);
#else
    return sqrtf(x);
#endif
}

float core_fast_rsqrt(float x) {
#if defined(__AVX2__)
    __m128 val = _mm_set_ss(x);
    val = _mm_rsqrt_ss(val);
    return _mm_cvtss_f32(val);
#elif defined(__ARM_NEON)
    float32x2_t val = vdup_n_f32(x);
    val = vrsqrte_f32(val);
    return vget_lane_f32(val, 0);
#else
    return 1.0f / sqrtf(x);
#endif
}

// Векторные операции с SIMD
void core_vector_sin(float* dst, const float* src, size_t n) {
#if defined(__AVX2__)
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 x = _mm256_loadu_ps(&src[i]);
        __m256 result;
        for (int j = 0; j < 8; ++j) {
            float val = _mm256_cvtss_f32(_mm256_permute_ps(x, j));
            result = _mm256_insert_ps(result, _mm_set_ss(core_fast_sin(val)), j << 4);
        }
        _mm256_storeu_ps(&dst[i], result);
    }
    for (; i < n; ++i) {
        dst[i] = core_fast_sin(src[i]);
    }
#elif defined(__ARM_NEON)
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t x = vld1q_f32(&src[i]);
        float32x4_t result;
        for (int j = 0; j < 4; ++j) {
            result = vsetq_lane_f32(core_fast_sin(vgetq_lane_f32(x, j)), result, j);
        }
        vst1q_f32(&dst[i], result);
    }
    for (; i < n; ++i) {
        dst[i] = core_fast_sin(src[i]);
    }
#else
    for (size_t i = 0; i < n; ++i) {
        dst[i] = core_fast_sin(src[i]);
    }
#endif
}

void core_vector_cos(float* dst, const float* src, size_t n) {
#if defined(__AVX2__)
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 x = _mm256_loadu_ps(&src[i]);
        __m256 result;
        for (int j = 0; j < 8; ++j) {
            float val = _mm256_cvtss_f32(_mm256_permute_ps(x, j));
            result = _mm256_insert_ps(result, _mm_set_ss(core_fast_cos(val)), j << 4);
        }
        _mm256_storeu_ps(&dst[i], result);
    }
    for (; i < n; ++i) {
        dst[i] = core_fast_cos(src[i]);
    }
#elif defined(__ARM_NEON)
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t x = vld1q_f32(&src[i]);
        float32x4_t result;
        for (int j = 0; j < 4; ++j) {
            result = vsetq_lane_f32(core_fast_cos(vgetq_lane_f32(x, j)), result, j);
        }
        vst1q_f32(&dst[i], result);
    }
    for (; i < n; ++i) {
        dst[i] = core_fast_cos(src[i]);
    }
#else
    for (size_t i = 0; i < n; ++i) {
        dst[i] = core_fast_cos(src[i]);
    }
#endif
}

void core_vector_sqrt(float* dst, const float* src, size_t n) {
#if defined(__AVX2__)
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 x = _mm256_loadu_ps(&src[i]);
        __m256 result = _mm256_sqrt_ps(x);
        _mm256_storeu_ps(&dst[i], result);
    }
    for (; i < n; ++i) {
        dst[i] = core_fast_sqrt(src[i]);
    }
#elif defined(__ARM_NEON)
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t x = vld1q_f32(&src[i]);
        float32x4_t result = vsqrtq_f32(x);
        vst1q_f32(&dst[i], result);
    }
    for (; i < n; ++i) {
        dst[i] = core_fast_sqrt(src[i]);
    }
#else
    for (size_t i = 0; i < n; ++i) {
        dst[i] = core_fast_sqrt(src[i]);
    }
#endif
}

void core_vector_rsqrt(float* dst, const float* src, size_t n) {
#if defined(__AVX2__)
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 x = _mm256_loadu_ps(&src[i]);
        __m256 result = _mm256_rsqrt_ps(x);
        _mm256_storeu_ps(&dst[i], result);
    }
    for (; i < n; ++i) {
        dst[i] = core_fast_rsqrt(src[i]);
    }
#elif defined(__ARM_NEON)
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t x = vld1q_f32(&src[i]);
        float32x4_t result = vrsqrteq_f32(x);
        vst1q_f32(&dst[i], result);
    }
    for (; i < n; ++i) {
        dst[i] = core_fast_rsqrt(src[i]);
    }
#else
    for (size_t i = 0; i < n; ++i) {
        dst[i] = core_fast_rsqrt(src[i]);
    }
#endif
}

// Матричные операции
void core_matrix_multiply_4x4(float* dst, const float* a, const float* b) {
#if defined(__AVX2__)
    for (int i = 0; i < 4; ++i) {
        __m256 row = _mm256_loadu_ps(&a[i * 4]);
        for (int j = 0; j < 4; ++j) {
            __m256 col = _mm256_loadu_ps(&b[j * 4]);
            __m256 prod = _mm256_mul_ps(row, col);
            float sum = 0;
            for (int k = 0; k < 4; ++k) {
                sum += _mm256_cvtss_f32(_mm256_permute_ps(prod, k));
            }
            dst[i * 4 + j] = sum;
        }
    }
#elif defined(__ARM_NEON)
    for (int i = 0; i < 4; ++i) {
        float32x4_t row = vld1q_f32(&a[i * 4]);
        for (int j = 0; j < 4; ++j) {
            float32x4_t col = vld1q_f32(&b[j * 4]);
            float32x4_t prod = vmulq_f32(row, col);
            float sum = 0;
            for (int k = 0; k < 4; ++k) {
                sum += vgetq_lane_f32(prod, k);
            }
            dst[i * 4 + j] = sum;
        }
    }
#else
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float sum = 0;
            for (int k = 0; k < 4; ++k) {
                sum += a[i * 4 + k] * b[k * 4 + j];
            }
            dst[i * 4 + j] = sum;
        }
    }
#endif
}

void core_matrix_transpose_4x4(float* dst, const float* src) {
#if defined(__AVX2__)
    __m256 row0 = _mm256_loadu_ps(&src[0]);
    __m256 row1 = _mm256_loadu_ps(&src[4]);
    __m256 row2 = _mm256_loadu_ps(&src[8]);
    __m256 row3 = _mm256_loadu_ps(&src[12]);
    
    __m256 tmp0 = _mm256_unpacklo_ps(row0, row1);
    __m256 tmp1 = _mm256_unpackhi_ps(row0, row1);
    __m256 tmp2 = _mm256_unpacklo_ps(row2, row3);
    __m256 tmp3 = _mm256_unpackhi_ps(row2, row3);
    
    row0 = _mm256_movelh_ps(tmp0, tmp2);
    row1 = _mm256_movehl_ps(tmp2, tmp0);
    row2 = _mm256_movelh_ps(tmp1, tmp3);
    row3 = _mm256_movehl_ps(tmp3, tmp1);
    
    _mm256_storeu_ps(&dst[0], row0);
    _mm256_storeu_ps(&dst[4], row1);
    _mm256_storeu_ps(&dst[8], row2);
    _mm256_storeu_ps(&dst[12], row3);
#elif defined(__ARM_NEON)
    float32x4x4_t mat = vld4q_f32(src);
    vst1q_f32(&dst[0], mat.val[0]);
    vst1q_f32(&dst[4], mat.val[1]);
    vst1q_f32(&dst[8], mat.val[2]);
    vst1q_f32(&dst[12], mat.val[3]);
#else
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            dst[i * 4 + j] = src[j * 4 + i];
        }
    }
#endif
}

// Оптимизированные операции с целыми числами
int32_t core_fast_div(int32_t a, int32_t b) {
    // Используем битовые операции для быстрого деления на степени 2
    if (b == 0) return 0;
    if (b == 1) return a;
    if (b == 2) return a >> 1;
    if (b == 4) return a >> 2;
    if (b == 8) return a >> 3;
    if (b == 16) return a >> 4;
    return a / b;
}

int32_t core_fast_mod(int32_t a, int32_t b) {
    // Используем битовые операции для быстрого остатка от деления на степени 2
    if (b == 0) return 0;
    if (b == 1) return 0;
    if (b == 2) return a & 1;
    if (b == 4) return a & 3;
    if (b == 8) return a & 7;
    if (b == 16) return a & 15;
    return a % b;
} 