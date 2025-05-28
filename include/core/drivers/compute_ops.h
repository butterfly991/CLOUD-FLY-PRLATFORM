#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// Оптимизированные математические операции
void core_compute_vector_add(const float* a, const float* b, float* result, size_t size);
void core_compute_vector_sub(const float* a, const float* b, float* result, size_t size);
void core_compute_vector_mul(const float* a, const float* b, float* result, size_t size);
void core_compute_vector_div(const float* a, const float* b, float* result, size_t size);
void core_compute_vector_scale(const float* a, float scale, float* result, size_t size);
void core_compute_vector_dot(const float* a, const float* b, float* result, size_t size);
void core_compute_vector_cross(const float* a, const float* b, float* result);
void core_compute_vector_normalize(float* vector, size_t size);
float core_compute_vector_length(const float* vector, size_t size);

// Оптимизированные матричные операции
void core_compute_matrix_add(const float* a, const float* b, float* result, size_t rows, size_t cols);
void core_compute_matrix_sub(const float* a, const float* b, float* result, size_t rows, size_t cols);
void core_compute_matrix_mul(const float* a, const float* b, float* result,
                           size_t a_rows, size_t a_cols, size_t b_cols);
void core_compute_matrix_transpose(const float* matrix, float* result, size_t rows, size_t cols);
void core_compute_matrix_inverse(const float* matrix, float* result, size_t size);
float core_compute_matrix_determinant(const float* matrix, size_t size);

// Оптимизированные операции с кватернионами
void core_compute_quaternion_mul(const float* a, const float* b, float* result);
void core_compute_quaternion_conjugate(const float* quat, float* result);
void core_compute_quaternion_normalize(float* quat);
void core_compute_quaternion_to_matrix(const float* quat, float* matrix);
void core_compute_matrix_to_quaternion(const float* matrix, float* quat);

// Оптимизированные операции с геометрией
void core_compute_ray_intersect_triangle(const float* ray_origin, const float* ray_direction,
                                       const float* v0, const float* v1, const float* v2,
                                       float* t, float* u, float* v);
void core_compute_ray_intersect_aabb(const float* ray_origin, const float* ray_direction,
                                   const float* aabb_min, const float* aabb_max,
                                   float* t_min, float* t_max);
void core_compute_ray_intersect_sphere(const float* ray_origin, const float* ray_direction,
                                     const float* sphere_center, float sphere_radius,
                                     float* t_min, float* t_max);

// Оптимизированные операции с кривыми
void core_compute_bezier_point(const float* control_points, size_t degree, float t, float* result);
void core_compute_bezier_derivative(const float* control_points, size_t degree, float t, float* result);
void core_compute_bspline_point(const float* control_points, const float* knots,
                              size_t degree, float t, float* result);
void core_compute_bspline_derivative(const float* control_points, const float* knots,
                                   size_t degree, float t, float* result);

// Оптимизированные операции с шумом
float core_compute_perlin_noise(float x, float y, float z);
float core_compute_simplex_noise(float x, float y, float z);
float core_compute_worley_noise(float x, float y, float z, int seed);
float core_compute_fractal_noise(float x, float y, float z, int octaves, float persistence);

// Оптимизированные операции с фильтрацией
void core_compute_gaussian_blur(const float* input, float* output, size_t width, size_t height,
                              float sigma);
void core_compute_bilateral_filter(const float* input, float* output, size_t width, size_t height,
                                 float sigma_space, float sigma_color);
void core_compute_median_filter(const float* input, float* output, size_t width, size_t height,
                              size_t kernel_size);

// Оптимизированные операции с преобразованиями
void core_compute_fft(const float* input, float* output, size_t size);
void core_compute_ifft(const float* input, float* output, size_t size);
void core_compute_dct(const float* input, float* output, size_t size);
void core_compute_idct(const float* input, float* output, size_t size);

#ifdef __cplusplus
}
#endif 