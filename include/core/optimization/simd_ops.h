#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// SIMD/AVX/NEON ускоренные операции
void core_vector_add_f32(float* dst, const float* src1, const float* src2, size_t n);
void core_vector_add_i32(int32_t* dst, const int32_t* src1, const int32_t* src2, size_t n);

#ifdef __cplusplus
}
#endif 