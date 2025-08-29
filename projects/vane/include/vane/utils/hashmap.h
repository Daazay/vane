#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/collection_common.h"

typedef struct Hashmap Hashmap;
typedef struct HashmapIterator HashmapIterator;
typedef struct HashItemSpecs HashmapKeySpecs;
typedef struct ItemSpecs HashmapValueSpecs;
typedef struct Vector HashmapBucket;

#define HASHMAP_KEY_SPECS(TYPE, HASH_FN, EQUALS_FN, DESTROY_FN) HASH_ITEM_SPECS(TYPE, HASH_FN, EQUALS_FN, DESTROY_FN)
#define HASHMAP_VALUE_SPECS(TYPE, DESTROY_FN) ITEM_SPECS(TYPE, DESTROY_FN)

#define HASHMAP_ENTRY_MAX_STACK_SIZE 64
#define HASHMAP_DEFAULT_CAPACITY     16
#define HASHMAP_INIT_BUCKET_SIZE     2
#define HASHMAP_CAPACITY_MULTIPLIER  4
#define HASHMAP_LOAD_FACTOR          0.75f
#define HASHMAP_GROWTH_FACTOR        2

struct Hashmap {
    HashmapKeySpecs key_specs;
    HashmapValueSpecs value_specs;
    HashmapBucket* buckets;
    u32 capacity;
    u32 size;
};

struct HashmapIterator {
    const Hashmap* map;
    u32 bucket_index;
    u32 item_index;
};

// -- creatrion --

Hashmap hashmap_create(u32 init_cap, HashmapKeySpecs key_specs, HashmapValueSpecs value_specs);

// -- destruction --

void hashmap_destroy(Hashmap* map);

void hashmap_clear(Hashmap* map);

// -- utility --

bool is_hashmap_empty(const Hashmap* map);

void hashmap_rehash(Hashmap* map, u32 new_cap);

void hashmap_shrink_to_fit(Hashmap* map);

// -- insertion --

void hashmap_insert(Hashmap* map, const void* key, const void* value);

// -- retrieval --

bool hashmap_contains(const Hashmap* map, const void* key);

void* hashmap_get(const Hashmap* map, const void* key);

const void* hashmap_get_key(const Hashmap* map, const void* key);

// -- removal --

bool hashmap_remove(Hashmap* map, const void* key);

// -- iteration --

HashmapIterator hashmap_get_it(const Hashmap* map);

bool hashmap_it_next(HashmapIterator* it, const void* key, void* value);