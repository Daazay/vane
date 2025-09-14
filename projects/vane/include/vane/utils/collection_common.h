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

static inline u32 item_ptr_hash(const void* ptr) {
    union { const void* p; u64 u; } cvt;
    cvt.u = 0;
    cvt.p = ptr;
    u64 x = cvt.u;

    // splitmix64-ish mix
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;

    return (u32)(x ^ (x >> 32));
}

static inline bool item_ptr_eq(const void* ptr1, const void* ptr2) {
    return ptr1 == ptr2;
}