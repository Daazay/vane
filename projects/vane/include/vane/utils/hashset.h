#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/collection_common.h"


typedef struct Hashset Hashset;
typedef struct HashsetIterator HashsetIterator;
typedef struct HashItemSpecs HashsetItemSpecs;
typedef struct Vector HashsetBucket;

#define HASHSET_ITEM_SPECS(TYPE, HASH_FN, EQUALS_FN, DESTROY_FN) HASH_ITEM_SPECS(TYPE, HASH_FN, EQUALS_FN, DESTROY_FN)

#define HASHSET_ENTRY_MAX_STACK_SIZE 64
#define HASHSET_DEFAULT_CAPACITY     16
#define HASHSET_INIT_BUCKET_SIZE     2
#define HASHSET_CAPACITY_MULTIPLIER  4
#define HASHSET_LOAD_FACTOR          0.75f
#define HASHSET_GROWTH_FACTOR        2

struct Hashset {
    HashsetItemSpecs item_specs;
    HashsetBucket* buckets;
    u32 capacity;
    u32 size;
};

struct HashsetIterator {
    const Hashset* set;
    u32 bucket_index;
    u32 item_index;
};

// -- creatrion --

Hashset hashset_create(u32 init_cap, HashsetItemSpecs item_specs);

// -- destruction --

void hashset_destroy(Hashset* set);

void hashset_clear(Hashset* set);

// -- utility --

bool is_hashset_empty(const Hashset* set);

void hashset_rehash(Hashset* set, u32 new_cap);

void hashset_shrink_to_fit(Hashset* set);

// -- insertion --

void hashset_insert(Hashset* set, const void* item);

// -- retrieval --

bool hashset_contains(const Hashset* set, const void* item);

const void* hashset_get(const Hashset* set, const void* item);

// -- removal --

bool hashset_remove(Hashset* set, const void* item);

// -- iteration --

HashsetIterator hashset_get_it(const Hashset* set);

bool hashset_it_next(HashsetIterator* it, const void* item);