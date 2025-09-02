#pragma once

#include "vane/utils/defines.h"

#define HASH_FNV_OFFSET_BASIS 0x811C9DC5U
#define HASH_MULTIPLIER       0x01000193U
#define HASH_FINALIZER_MUL    0x3243F6A9U
#define HASH_GOLDEN_RATIO_32  0x9E3779B9U

static inline u32 hash_finalizer(u32 hash) {
    hash ^= hash >> 15;
    hash *= HASH_FINALIZER_MUL;
    hash ^= hash >> 13;
    return hash;
}

static inline u32 hash_fnv1a(const u8* buf, u64 len) {
    if (buf == NULL || len == 0) {
        return 0;
    }

    u32 hash = HASH_FNV_OFFSET_BASIS;

    for (const u8* end = buf + len; buf < end; buf++) {
        hash ^= *buf;
        hash *= HASH_MULTIPLIER;
    }

    return hash_finalizer(hash);
}

static inline u32 hash_combine_u32(u32 seed, u32 value) {
    return seed ^ (value + HASH_GOLDEN_RATIO_32 + (seed << 6) + (seed >> 2));
}