#include "core/error_handling/core_errors.h"

const char* core_strerror(int code) {
    switch (code) {
        case CORE_SUCCESS: return "Success";
        case CORE_ERR_NOMEM: return "Out of memory";
        case CORE_ERR_INVALID: return "Invalid argument";
        case CORE_ERR_INTERNAL: return "Internal error";
        case CORE_ERR_NOTFOUND: return "Not found";
        case CORE_ERR_UNSUPPORTED: return "Unsupported operation";
        default: return "Unknown error";
    }
} 