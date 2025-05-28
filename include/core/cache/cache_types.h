#pragma once
#include <stddef.h>
#include <stdint.h>

#define CORE_CACHE_LINE_SIZE 64
#define CORE_CACHE_MAX_LEVELS 3

typedef struct {
    size_t size;
    size_t associativity;
    size_t line_size;
} core_cache_level_config_t;

typedef struct {
    core_cache_level_config_t levels[CORE_CACHE_MAX_LEVELS];
    size_t num_levels;
} core_cache_config_t; 