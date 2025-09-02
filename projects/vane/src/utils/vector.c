#include "vane/utils/vector.h"

#include <string.h>
#include <stdlib.h>

// -- creation --

Vector vector_create(u32 init_cap, VectorItemSpecs item_specs) {
    assert(item_specs.size > 0);

    u32 cap = (init_cap > 0)
        ? init_cap
        : VECTOR_DEFAULT_CAPACITY;

    if (cap < 2) {
        cap = 2;
    }

    u8* raw = malloc((u64)cap * item_specs.size);
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
            void* item = vector_at(*vec, i);
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

    vec->size = 0;
    vec->cap = 0;
}

// -- utilities --

bool is_vector_empty(Vector vec) {
    return vec.raw == NULL || vec.size == 0;
}

void vector_reserve(Vector* vec, u32 additional_size) {
    assert(vec != NULL);

    const u64 required = vec->size + additional_size;
    if (required <= vec->cap) {
        return;
    }

    u32 new_cap = (vec->cap > 0)
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

    u8* raw = realloc(vec->raw, (u64)new_cap * vec->item_specs.size);
    assert(raw != NULL);

    vec->raw = raw;
    vec->cap = (u32)new_cap;
}

void vector_resize(Vector* vec, u32 new_size) {
    assert(vec != NULL);

    if (new_size < vec->size) {
        if (vec->item_specs.destroy_fn != NULL) {
            for (u32 i = new_size; i < vec->size; i++) {
                void* item = vector_at(*vec, i);
                vec->item_specs.destroy_fn(item);
            }
        }
        vec->size = new_size;
        vector_shrink_to_fit(vec);
    }
    else if (new_size > vec->size) {
        vector_reserve(vec, new_size - vec->size);
    }
}

void vector_shrink_to_fit(Vector* vec) {
    assert(vec != NULL);

    if (vec->raw == NULL || vec->size == 0 || vec->size == vec->cap) {
        return;
    }

    u8* raw = realloc(vec->raw, vec->size * vec->item_specs.size);
    assert(raw != NULL);

    vec->raw = raw;
    vec->cap = vec->size;
}

// -- insertion --

void vector_insert(Vector* vec, u32 idx, const void* items, u32 count) {
    assert(vec != NULL);

    if (count == 0) {
        return;
    }

    assert((items != NULL || vec->item_specs.is_ptr) && "Items can be NULL only if item type is ptr");
    vector_reserve(vec, count);

    u8* insert_pos = vec->raw + idx * vec->item_specs.size;

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

// -- removal --

void vector_remove(Vector* vec, u32 idx) {
    assert(vec != NULL && idx < vec->size);

    u8* item = vec->raw + idx * vec->item_specs.size;

    if (vec->item_specs.destroy_fn != NULL) {
        vec->item_specs.destroy_fn(ITEM_SPEC_CAST(vec->item_specs.is_ptr, item));
    }

    if (idx < vec->size - 1) {
        u8* next_item = item + vec->item_specs.size;
        u64 bytes_to_move = (u64)(vec->size - idx - 1) * vec->item_specs.size;
        memmove(item, next_item, bytes_to_move);
    }

    vec->size--;
}

// -- access --

void* vector_at(Vector vec, u32 idx) {
    assert(idx < vec.size);
    void* item = vec.raw + (u64)idx * vec.item_specs.size;
    return ITEM_SPEC_CAST(vec.item_specs.is_ptr, item);
}

void vector_get(Vector vec, u32 idx, void* _item) {
    assert(idx < vec.size && _item != NULL);

    void* item = vec.raw + (u64)idx * vec.item_specs.size;
    memcpy(_item, item, vec.item_specs.size);
}

// -- modification --

void vector_set(Vector vec, u32 idx, const void* item) {
    assert(idx < vec.size);
    assert((item != NULL || vec.item_specs.is_ptr) && "item can be NULL only if item type is ptr");

    void* dst = vec.raw + idx * vec.item_specs.size;

    if (vec.item_specs.destroy_fn != NULL) {
        vec.item_specs.destroy_fn(ITEM_SPEC_CAST(vec.item_specs.is_ptr, dst));
    }

    if (item != NULL) {
        memcpy(dst, item, vec.item_specs.size);
    }
    else {
        memset(dst, 0, vec.item_specs.size);
    }
}