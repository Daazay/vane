#pragma once

#include "vane/utils/defines.h"

typedef void(*ItemDestroyFn)(void* item);
typedef bool(*ItemEqualsFn)(const void* a, const void* b);
typedef u32(*ItemHashFn)(const void* item);

#define IS_TYPE_PTR(TYPE) ((#TYPE)[sizeof(#TYPE) / sizeof(char) - 2] == '*')
#define ITEM_SPECS_CAST(ITEM_SPECS, ITEM) (((ITEM_SPECS).is_ptr) ? *(void**)ITEM : ITEM)

#define ITEM_SPECS(TYPE, DESTROY_FN) ((struct ItemSpecs) { \
    .size = sizeof(TYPE), \
    .is_ptr = IS_TYPE_PTR(TYPE), \
    .destroy_fn = (ItemDestroyFn)DESTROY_FN, \
})

#define HASH_ITEM_SPECS(TYPE, HASH_FN, EQUALS_FN, DESTROY_FN) ((struct HashItemSpecs) { \
    .size = sizeof(TYPE), \
    .is_ptr = IS_TYPE_PTR(TYPE), \
    .destroy_fn = (ItemDestroyFn)DESTROY_FN, \
    .equals_fn = (ItemEqualsFn)EQUALS_FN, \
    .hash_fn = (ItemHashFn)HASH_FN, \
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
    ItemEqualsFn equals_fn;
    ItemHashFn hash_fn;
};