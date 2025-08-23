#pragma once

#include "vane/utils/defines.h"

typedef void(*ItemDestroyFn)(void* item);

#define IS_TYPE_PTR(TYPE) ((#TYPE)[sizeof(#TYPE) / sizeof(char) - 2] == '*')
#define ITEM_SPECS_CAST(ITEM_SPECS, ITEM) (((ITEM_SPECS).is_ptr) ? *(void**)ITEM : ITEM)

#define ITEM_SPECS(TYPE, DESTROY_FN) ((struct ItemSpecs) { \
    .size = sizeof(TYPE), \
    .is_ptr = IS_TYPE_PTR(TYPE), \
    .destroy_fn = (ItemDestroyFn)DESTROY_FN, \
})

struct ItemSpecs {
    u32 size;
    bool is_ptr;
    ItemDestroyFn destroy_fn;
};