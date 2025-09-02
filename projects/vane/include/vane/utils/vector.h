#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/collection_common.h"

typedef struct Vector Vector;
typedef struct ItemSpecs VectorItemSpecs;

#define VECTOR_DEFAULT_CAPACITY 32
#define VECTOR_CAPACITY_MULT    1.5
#define VECTOR_SPECS(TYPE, DESTROY_FN) ITEM_SPECS(TYPE, DESTROY_FN)

struct Vector {
    VectorItemSpecs item_specs;
    u8* raw;
    u32 size;
    u32 cap;
};

// -- creation --

Vector vector_create(u32 init_cap, VectorItemSpecs item_specs);

// -- destruction --

void vector_clear(Vector* vec);

void vector_destroy(Vector* vec);

// -- utilities --

bool is_vector_empty(Vector vec);

void vector_reserve(Vector* vec, u32 additional_size);

void vector_resize(Vector* vec, u32 new_size);

void vector_shrink_to_fit(Vector* vec);

// -- insertion --

void vector_insert(Vector* vec, u32 idx, const void* items, u32 count);

static inline void vector_push_back(Vector* vec, const void* item) {
    assert(vec != NULL);
    vector_insert(vec, vec->size, item, 1);
}

static inline void vector_push_front(Vector* vec, const void* item) {
    assert(vec != NULL);
    vector_insert(vec, 0, item, 1);
}

static inline void vector_extend_back(Vector* vec, const void* items, u32 count) {
    assert(vec != NULL);
    vector_insert(vec, vec->size, items, count);
}

static inline void vector_extend_front(Vector* vec, const void* items, u32 count) {
    assert(vec != NULL);
    vector_insert(vec, 0, items, count);
}

// -- removal --

void vector_remove(Vector* vec, u32 idx);

static inline void vector_pop_front(Vector* vec) {
    assert(vec != NULL && vec->size > 0);
    vector_remove(vec, 0);
}

static inline void vector_pop_back(Vector* vec) {
    assert(vec != NULL && vec->size > 0);
    vector_remove(vec, vec->size - 1);
}

// -- access --

void* vector_at(Vector vec, u32 idx);

static inline void* vector_at_back(Vector vec) {
    assert(vec.size > 0);
    return vector_at(vec, vec.size - 1);
}

static inline void* vector_at_front(Vector vec) {
    assert(vec.size > 0);
    return vector_at(vec, 0);
}

void vector_get(Vector vec, u32 idx, void* item);

static inline void vector_get_back(Vector vec, void* _item) {
    assert(vec.size > 0 && _item != NULL);
    vector_get(vec, vec.size - 1, _item);
}

static inline void vector_get_front(Vector vec, void* _item) {
    assert(vec.size > 0 && _item != NULL);
    vector_get(vec, 0, _item);
}

// -- modification --

void vector_set(Vector vec, u32 idx, const void* item);