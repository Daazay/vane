#pragma once

#include "vane/utils/defines.h"

typedef void(*ItemDestroyFn)(void* item);
typedef bool(*ItemEqualsFn)(const void* a, const void* b);
typedef u32(*ItemHashFn)(const void* item);

#define IS_TYPE_PTR(TYPE) ((#TYPE)[sizeof(#TYPE) - 2] == '*')
#define ITEM_SPEC_CAST(IS_PTR, ITEM) (IS_PTR ? *(void**)ITEM : ITEM)

#define ITEM_SPECS(TYPE, DESTROY_FN) ((struct ItemSpecs) { \
    .size = sizeof(TYPE), \
    .is_ptr = IS_TYPE_PTR(TYPE), \
    .destroy_fn = (ItemDestroyFn)(DESTROY_FN), \
})

#define HASH_ITEM_SPECS(TYPE, HASH_FN, EQUALS_FN, DESTROY_FN) ((struct HashItemSpecs) { \
    .size = sizeof(TYPE), \
    .is_ptr = IS_TYPE_PTR(TYPE), \
    .destroy_fn = (ItemDestroyFn)(DESTROY_FN), \
    .hash_fn = (ItemHashFn)(HASH_FN), \
    .equals_fn = (ItemEqualsFn)(EQUALS_FN), \
})

struct ItemSpecs {
    u32 size;
    bool is_ptr;
    ItemDestroyFn destroy_fn;
};

struct HashItemSpecs {
    u32 size;
    bool is_ptr;
    ItemDestroyFn destroy_fn;
    ItemHashFn hash_fn;
    ItemEqualsFn equals_fn;
};