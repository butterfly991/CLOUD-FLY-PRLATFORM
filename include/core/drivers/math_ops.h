#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Оптимизированные математические операции
float core_fast_sin(float x);
float core_fast_cos(float x);
float core_fast_sqrt(float x);
float core_fast_rsqrt(float x); // Быстрое обратное значение квадратного корня

// Векторные математические операции
void core_vector_sin(float* dst, const float* src, size_t n);
void core_vector_cos(float* dst, const float* src, size_t n);
void core_vector_sqrt(float* dst, const float* src, size_t n);
void core_vector_rsqrt(float* dst, const float* src, size_t n);

// Матричные операции
void core_matrix_multiply_4x4(float* dst, const float* a, const float* b);
void core_matrix_transpose_4x4(float* dst, const float* src);

// Оптимизированные операции с целыми числами
int32_t core_fast_div(int32_t a, int32_t b);
int32_t core_fast_mod(int32_t a, int32_t b);

#ifdef __cplusplus
}
#endif 