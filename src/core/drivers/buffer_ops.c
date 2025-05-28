#include "core/drivers/buffer_ops.h"
#include "core/drivers/memory_ops.h"
#include "core/drivers/thread_ops.h"

// Операции с буферами
core_buffer_t* core_buffer_create(size_t initial_capacity) {
    core_buffer_t* buffer = (core_buffer_t*)core_malloc(sizeof(core_buffer_t));
    if (!buffer) {
        return NULL;
    }

    buffer->data = core_malloc(initial_capacity);
    if (!buffer->data) {
        core_free(buffer);
        return NULL;
    }

    buffer->capacity = initial_capacity;
    buffer->size = 0;
    buffer->position = 0;
    buffer->is_owner = 1;

    return buffer;
}

void core_buffer_destroy(core_buffer_t* buffer) {
    if (!buffer) {
        return;
    }

    if (buffer->is_owner && buffer->data) {
        core_free(buffer->data);
    }

    core_free(buffer);
}

int core_buffer_resize(core_buffer_t* buffer, size_t new_capacity) {
    if (!buffer || !buffer->is_owner) {
        return -1;
    }

    void* new_data = core_malloc(new_capacity);
    if (!new_data) {
        return -1;
    }

    // Копируем существующие данные
    size_t copy_size = buffer->size < new_capacity ? buffer->size : new_capacity;
    core_memcpy_aligned(new_data, buffer->data, copy_size);

    // Освобождаем старые данные
    core_free(buffer->data);

    buffer->data = new_data;
    buffer->capacity = new_capacity;
    if (buffer->size > new_capacity) {
        buffer->size = new_capacity;
    }
    if (buffer->position > new_capacity) {
        buffer->position = new_capacity;
    }

    return 0;
}

void core_buffer_clear(core_buffer_t* buffer) {
    if (!buffer) {
        return;
    }

    buffer->size = 0;
    buffer->position = 0;
}

// Операции с данными
int core_buffer_write(core_buffer_t* buffer, const void* data, size_t size) {
    if (!buffer || !data || size == 0) {
        return -1;
    }

    // Проверяем, нужно ли увеличить буфер
    if (buffer->position + size > buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        while (new_capacity < buffer->position + size) {
            new_capacity *= 2;
        }
        if (core_buffer_resize(buffer, new_capacity) != 0) {
            return -1;
        }
    }

    // Копируем данные
    core_memcpy_aligned((char*)buffer->data + buffer->position, data, size);
    buffer->position += size;
    if (buffer->position > buffer->size) {
        buffer->size = buffer->position;
    }

    return 0;
}

int core_buffer_read(core_buffer_t* buffer, void* data, size_t size) {
    if (!buffer || !data || size == 0) {
        return -1;
    }

    // Проверяем, достаточно ли данных
    if (buffer->position + size > buffer->size) {
        return -1;
    }

    // Копируем данные
    core_memcpy_aligned(data, (char*)buffer->data + buffer->position, size);
    buffer->position += size;

    return 0;
}

int core_buffer_peek(core_buffer_t* buffer, void* data, size_t size) {
    if (!buffer || !data || size == 0) {
        return -1;
    }

    // Проверяем, достаточно ли данных
    if (buffer->position + size > buffer->size) {
        return -1;
    }

    // Копируем данные без изменения позиции
    core_memcpy_aligned(data, (char*)buffer->data + buffer->position, size);

    return 0;
}

int core_buffer_skip(core_buffer_t* buffer, size_t size) {
    if (!buffer || size == 0) {
        return -1;
    }

    // Проверяем, достаточно ли данных
    if (buffer->position + size > buffer->size) {
        return -1;
    }

    buffer->position += size;
    return 0;
}

// Операции с позицией
int core_buffer_seek(core_buffer_t* buffer, size_t position) {
    if (!buffer || position > buffer->size) {
        return -1;
    }

    buffer->position = position;
    return 0;
}

size_t core_buffer_tell(core_buffer_t* buffer) {
    return buffer ? buffer->position : 0;
}

int core_buffer_rewind(core_buffer_t* buffer) {
    if (!buffer) {
        return -1;
    }

    buffer->position = 0;
    return 0;
}

// Операции с памятью
void* core_buffer_data(core_buffer_t* buffer) {
    return buffer ? buffer->data : NULL;
}

size_t core_buffer_size(core_buffer_t* buffer) {
    return buffer ? buffer->size : 0;
}

size_t core_buffer_capacity(core_buffer_t* buffer) {
    return buffer ? buffer->capacity : 0;
}

int core_buffer_is_empty(core_buffer_t* buffer) {
    return buffer ? (buffer->size == 0) : 1;
}

int core_buffer_is_full(core_buffer_t* buffer) {
    return buffer ? (buffer->size == buffer->capacity) : 0;
}

// Операции с копированием
int core_buffer_copy(core_buffer_t* dest, const core_buffer_t* src) {
    if (!dest || !src) {
        return -1;
    }

    // Проверяем, нужно ли увеличить буфер
    if (src->size > dest->capacity) {
        if (core_buffer_resize(dest, src->size) != 0) {
            return -1;
        }
    }

    // Копируем данные
    core_memcpy_aligned(dest->data, src->data, src->size);
    dest->size = src->size;
    dest->position = 0;

    return 0;
}

int core_buffer_clone(core_buffer_t* dest, const core_buffer_t* src) {
    if (!dest || !src) {
        return -1;
    }

    // Создаем новый буфер
    core_buffer_t* new_buffer = core_buffer_create(src->size);
    if (!new_buffer) {
        return -1;
    }

    // Копируем данные
    core_memcpy_aligned(new_buffer->data, src->data, src->size);
    new_buffer->size = src->size;
    new_buffer->position = 0;

    // Освобождаем старый буфер
    core_buffer_destroy(dest);

    // Устанавливаем новый буфер
    *dest = *new_buffer;
    core_free(new_buffer);

    return 0;
}

// Операции с выравниванием
int core_buffer_align(core_buffer_t* buffer, size_t alignment) {
    if (!buffer || alignment == 0) {
        return -1;
    }

    size_t aligned_size = (buffer->size + alignment - 1) & ~(alignment - 1);
    if (aligned_size > buffer->capacity) {
        if (core_buffer_resize(buffer, aligned_size) != 0) {
            return -1;
        }
    }

    buffer->size = aligned_size;
    return 0;
}

int core_buffer_is_aligned(core_buffer_t* buffer, size_t alignment) {
    if (!buffer || alignment == 0) {
        return 0;
    }

    return ((uintptr_t)buffer->data & (alignment - 1)) == 0;
}

// Операции с кэшем
void core_buffer_prefetch(core_buffer_t* buffer) {
    if (!buffer || !buffer->data) {
        return;
    }

    core_prefetch(buffer->data);
}

void core_buffer_flush(core_buffer_t* buffer) {
    if (!buffer || !buffer->data) {
        return;
    }

    core_flush_cache_line(buffer->data);
}

void core_buffer_invalidate(core_buffer_t* buffer) {
    if (!buffer || !buffer->data) {
        return;
    }

    core_invalidate_cache_line(buffer->data);
} 