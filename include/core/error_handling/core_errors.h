#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Коды ошибок ядра
#define CORE_SUCCESS 0
#define CORE_ERR_NOMEM 1
#define CORE_ERR_INVALID 2
#define CORE_ERR_INTERNAL 3
#define CORE_ERR_NOTFOUND 4
#define CORE_ERR_UNSUPPORTED 5

const char* core_strerror(int code);

#ifdef __cplusplus
}
#endif 