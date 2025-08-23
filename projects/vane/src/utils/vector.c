#include "vane/utils/vector.h"

#include <string.h>
#include <stdlib.h>

// -- creation --

Vector vector_create(u32 init_cap, VectorItemSpecs item_specs) {
    assert(item_specs.size > 0);

    u32 cap = (init_cap > 0)
        ? init_cap
        : VECTOR_DEFAULT_CAPACITY;

    byte* raw = malloc((u64)cap * item_specs.size);
    assert(raw != NULL);

    return (Vector) {
        .raw = raw,
        .size = 0,
        .cap = cap,
        .item_specs = item_specs,
    };
}

// -- destruction --

void vector_clear(Vector* vec) {
    assert(vec != NULL);

    if (vec->item_specs.destroy_fn != NULL) {
        for (u32 i = 0; i < vec->size; ++i) {
            void* item = vector_at(vec, i);
            vec->item_specs.destroy_fn(item);
        }
    }

    vec->size = 0;
}

void vector_destroy(Vector* vec) {
    if (vec == NULL) {
        return;
    }

    if (vec->raw != NULL) {
        vector_clear(vec);

        free(vec->raw);
        vec->raw = NULL;
    }

    vec->cap = 0;
    vec->size = 0;
}

// -- utilities --

bool is_vector_empty(const Vector* vec) {
    assert(vec != NULL);
    return vec->raw == NULL || vec->size == 0;
}

// -- insertion --

static inline void vector_ensure_capacity(Vector* vec, u32 appended_size) {
    const u64 required = vec->size + appended_size;
    if (required <= vec->cap) {
        return;
    }

    u32 new_cap = (vec->size > 0)
        ? vec->cap
        : VECTOR_DEFAULT_CAPACITY;

    if (new_cap == 1) {
        new_cap = 2;
    }

    while (new_cap < required) {
        const u32 next_cap = (u32)((f32)new_cap * VECTOR_CAPACITY_MULT);
        assert(next_cap > new_cap && "Capacity growth overflow");

        new_cap = next_cap;
    }

    u8* new_raw = realloc(vec->raw, (u64)new_cap * vec->item_specs.size);
    assert(new_raw != NULL);

    vec->raw = new_raw;
    vec->cap = new_cap;
}

void vector_insert(Vector* vec, u32 idx, const void* items, u32 count) {
    assert(vec != NULL);

    if (count == 0) {
        return;
    }

    assert((items != NULL || vec->item_specs.is_ptr) && "Items can be NULL only if item type is ptr");
    vector_ensure_capacity(vec, count);

    byte* insert_pos = vec->raw + idx * vec->item_specs.size;

    if (idx < vec->size) {
        u64 move_bytes = (u64)(vec->size - idx) * vec->item_specs.size;
        memmove(insert_pos + (u64)count * vec->item_specs.size, insert_pos, move_bytes);
    }

    if (items != NULL) {
        memcpy(insert_pos, items, (u64)count * vec->item_specs.size);
    }
    else {
        memset(insert_pos, 0, (u64)count * vec->item_specs.size);
    }

    vec->size += count;
}

void vector_push_front(Vector* vec, const void* item) {
    assert(vec != NULL);
    vector_insert(vec, 0, item, 1);
}

void vector_push_back(Vector* vec, const void* item) {
    assert(vec != NULL);
    vector_insert(vec, vec->size, item, 1);
}

void vector_extend(Vector* vec, const void* items, u32 count) {
    assert(vec != NULL);
    vector_insert(vec, vec->size, items, count);
}

// -- removal --

void vector_remove(Vector* vec, u32 idx) {
    assert(vec != NULL && idx < vec->size);

    byte* item = vec->raw + idx * vec->item_specs.size;

    if (vec->item_specs.destroy_fn != NULL) {
        vec->item_specs.destroy_fn(ITEM_SPECS_CAST(vec->item_specs, item));
    }

    if (idx < vec->size - 1) {
        byte* next_item = item + vec->item_specs.size;
        u64 bytes_to_move = (u64)(vec->size - idx - 1) * vec->item_specs.size;
        memmove(item, next_item, bytes_to_move);
    }

    vec->size--;
}

void vector_pop_front(Vector* vec) {
    assert(vec != NULL && vec->size > 0);
    vector_remove(vec, 0);
}

void vector_pop_back(Vector* vec) {
    assert(vec != NULL && vec->size > 0);
    vector_remove(vec, vec->size - 1);
}

// -- Access --

void* vector_at(const Vector* vec, u32 idx) {
    assert(vec != NULL && idx < vec->size);

    void* item = vec->raw + (u64)idx * vec->item_specs.size;
    return ITEM_SPECS_CAST(vec->item_specs, item);
}

void* vector_at_front(const Vector* vec) {
    assert(vec != NULL && vec->size > 0);
    return vector_at(vec, 0);
}

void* vector_at_back(const Vector* vec) {
    assert(vec != NULL && vec->size > 0);
    return vector_at(vec, vec->size - 1);
}

// -- Modification --

void vector_set(Vector* vec, u32 idx, const void* item) {
    assert(vec != NULL && idx < vec->size);
    assert((item != NULL || vec->item_specs.is_ptr) && "item can be NULL only if item type is ptr");

    void* dst = vec->raw + idx * vec->item_specs.size;

    if (vec->item_specs.destroy_fn != NULL) {
        vec->item_specs.destroy_fn(ITEM_SPECS_CAST(vec->item_specs, dst));
    }

    if (item != NULL) {
        memcpy(dst, item, vec->item_specs.size);
    }
    else {
        memset(dst, 0, vec->item_specs.size);
    }
}