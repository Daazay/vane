#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/collection_common.h"

typedef struct Vector Vector;
typedef struct ItemSpecs VectorItemSpecs;

#define VECTOR_DEFAULT_CAPACITY 32
#define VECTOR_CAPACITY_MULT    1.5
#define VECTOR_ITEM_SPECS(TYPE, DESTROY_FN) ITEM_SPECS(TYPE, DESTROY_FN)

struct Vector {
    VectorItemSpecs item_specs;
    byte* raw;
    u32 size;
    u32 cap;
};

// -- creation --

Vector vector_create(u32 init_cap, VectorItemSpecs item_specs);

// -- destruction --

void vector_clear(Vector* vec);

void vector_destroy(Vector* vec);

// -- utilities --

bool is_vector_empty(const Vector* vec);

// -- insertion --

void vector_insert(Vector* vec, u32 idx, const void* items, u32 count);

void vector_push_front(Vector* vec, const void* item);

void vector_push_back(Vector* vec, const void* item);

void vector_extend(Vector* vec, const void* items, u32 count);

// -- removal --

void vector_remove(Vector* vec, u32 idx);

void vector_pop_front(Vector* vec);

void vector_pop_back(Vector* vec);

// -- Access --

void* vector_at(const Vector* vec, u32 idx);

void* vector_at_front(const Vector* vec);

void* vector_at_back(const Vector* vec);

// -- Modification --

void vector_set(Vector* vec, u32 idx, const void* item);