#include "vane/utils/hashmap.h"

#include <stdlib.h>
#include <string.h>

#include "vane/utils/vector.h"

typedef struct HashmapEntry HashmapEntry;
struct HashmapEntry {
    u32 hash;
    void* data;
};

static inline u64 hashmap_entry_size(const Hashmap* map) {
    return sizeof(u32) + map->key_specs.size + map->value_specs.size;
}

static inline void* hashmap_entry_key(const Hashmap* map, const HashmapEntry* entry) {
    (void)map;
    return (u8*)entry + sizeof(u32);
}

static inline void* hashmap_entry_value(const Hashmap* map, const HashmapEntry* entry) {
    return (u8*)entry + sizeof(u32) + map->key_specs.size;
}

static inline void hashmap_entry_destroy(const Hashmap* map, HashmapEntry* entry) {
    if (map->key_specs.destroy_fn != NULL) {
        void* key = hashmap_entry_key(map, entry);
        void* actual_key = ITEM_SPEC_CAST(map->key_specs.is_ptr, key);
        map->key_specs.destroy_fn(actual_key);
    }
    if (map->value_specs.destroy_fn != NULL) {
        void* value = hashmap_entry_value(map, entry);
        void* actual_value = ITEM_SPEC_CAST(map->value_specs.is_ptr, value);
        map->value_specs.destroy_fn(actual_value);
    }
}

static inline void hashmap_bucket_init(const Hashmap* map, HashmapBucket* bucket) {
    const u64 entry_size = hashmap_entry_size(map);
    const bool is_entry_big = entry_size > HASHMAP_ENTRY_MAX_STACK_SIZE;

    *bucket = vector_create(HASHMAP_INIT_BUCKET_SIZE, (VectorItemSpecs) {
        .size = (u32)(is_entry_big ? sizeof(HashmapEntry*) : entry_size),
            .is_ptr = is_entry_big,
            .destroy_fn = is_entry_big ? &free : NULL,
    });
}

static inline HashmapEntry* hashmap_find_entry(const Hashmap* map, const void* key) {
    const void* actual_key = ITEM_SPEC_CAST(map->key_specs.is_ptr, key);
    const u32 computed_hash = map->key_specs.hash_fn(actual_key);
    const u32 bucket_idx = computed_hash % map->capacity;

    const HashmapBucket* bucket = &map->buckets[bucket_idx];
    if (bucket->raw == NULL) {
        return NULL;
    }

    for (u32 i = 0; i < bucket->size; i++) {
        HashmapEntry* entry = vector_at(*bucket, i);

        if (computed_hash == entry->hash) {
            void* entry_key = hashmap_entry_key(map, entry);
            void* actual_entry_key = ITEM_SPEC_CAST(map->key_specs.is_ptr, entry_key);
            if (map->key_specs.equals_fn(actual_key, actual_entry_key)) {
                return entry;
            }
        }
    }

    return NULL;
}

// -- creatrion --

Hashmap hashmap_create(u32 init_cap, HashmapKeySpecs key_specs, HashmapValueSpecs value_specs) {
    assert(key_specs.size > 0 && key_specs.hash_fn != NULL && key_specs.equals_fn != NULL);
    assert(value_specs.size > 0);

    u32 cap = (init_cap > 2)
        ? init_cap
        : HASHMAP_DEFAULT_CAPACITY;

    HashmapBucket* buckets = calloc(cap, sizeof(HashmapBucket));
    assert(buckets != NULL);

    return (Hashmap) {
        .key_specs = key_specs,
            .value_specs = value_specs,
            .buckets = buckets,
            .capacity = cap,
            .size = 0,
    };
}

// -- destruction --

void hashmap_destroy(Hashmap* map) {
    if (map == NULL || map->buckets == NULL) {
        return;
    }

    hashmap_clear(map);

    free(map->buckets);

    map->buckets = NULL;
    map->capacity = 0;
    map->size = 0;
}

void hashmap_clear(Hashmap* map) {
    assert(map != NULL);

    for (u32 i = 0; i < map->capacity; i++) {
        HashmapBucket* bucket = &map->buckets[i];
        if (bucket->raw == NULL) {
            continue;
        }

        for (u32 j = 0; j < bucket->size; j++) {
            HashmapEntry* entry = vector_at(*bucket, j);
            hashmap_entry_destroy(map, entry);
        }
        vector_destroy(bucket);
    }

    map->size = 0;
}

// -- utility --

bool is_hashmap_empty(const Hashmap* map) {
    assert(map != NULL);
    return map->size == 0;
}

void hashmap_rehash(Hashmap* map, u32 new_cap) {
    assert(map != NULL && new_cap > 0);

    if (new_cap == map->capacity) {
        return;
    }

    HashmapBucket* new_buckets = calloc(new_cap, sizeof(HashmapBucket));
    assert(new_buckets != NULL);

    for (u32 i = 0; i < map->capacity; i++) {
        HashmapBucket* old_bucket = &map->buckets[i];
        if (old_bucket->raw == NULL) {
            continue;
        }

        for (u32 j = 0; j < old_bucket->size; j++) {
            HashmapEntry* entry = vector_at(*old_bucket, j);

            const u32 new_bucket_idx = entry->hash % new_cap;
            HashmapBucket* new_bucket = &new_buckets[new_bucket_idx];
            if (new_bucket->raw == NULL) {
                hashmap_bucket_init(map, new_bucket);
            }

            vector_push_back(new_bucket, entry);
        }
        vector_destroy(old_bucket);
    }

    free(map->buckets);
    map->buckets = new_buckets;
    map->capacity = new_cap;
}

void hashmap_shrink_to_fit(Hashmap* map) {
    assert(map != NULL);

    if (map->size == 0) {
        if (map->capacity > HASHMAP_DEFAULT_CAPACITY) {
            hashmap_rehash(map, HASHMAP_DEFAULT_CAPACITY);
        }
        return;
    }

    // Find the minimal capacity such that size <= cap * load_factor
    u32 min_cap = HASHMAP_DEFAULT_CAPACITY;
    while ((u32)(min_cap * HASHMAP_LOAD_FACTOR) < map->size) {
        min_cap *= HASHMAP_GROWTH_FACTOR;
    }

    if (min_cap < map->capacity) {
        hashmap_rehash(map, min_cap);
    }
}

// -- insertion --

void hashmap_insert(Hashmap* map, const void* key, const void* value) {
    assert(map != NULL);
    assert(map->key_specs.is_ptr || (key != NULL && "key can be NULL only if item type is ptr"));
    assert(map->value_specs.is_ptr || (value != NULL && "value can be NULL only if item type is ptr"));

    HashmapEntry* existing_entry = hashmap_find_entry(map, key);
    if (existing_entry != NULL) {
        // Update existing entry
        void* entry_value = hashmap_entry_value(map, existing_entry);
        void* actual_entry_value = ITEM_SPEC_CAST(map->value_specs.is_ptr, entry_value);

        if (map->value_specs.destroy_fn != NULL) {
            map->value_specs.destroy_fn(actual_entry_value);
        }
        if (value == NULL) {
            memset(entry_value, 0, map->value_specs.size);
        }
        else {
            memcpy(entry_value, value, map->value_specs.size);
        }
        return;
    }

    if (map->size >= map->capacity * HASHMAP_LOAD_FACTOR) {
        hashmap_rehash(map, map->capacity * HASHMAP_GROWTH_FACTOR);
    }

    const void* actual_key = ITEM_SPEC_CAST(map->key_specs.is_ptr, key);
    const u32 computed_hash = map->key_specs.hash_fn(actual_key);
    const u32 bucket_idx = computed_hash % map->capacity;

    HashmapBucket* bucket = &map->buckets[bucket_idx];
    if (bucket->raw == NULL) {
        hashmap_bucket_init(map, bucket);
    }

    const u64 entry_size = hashmap_entry_size(map);

    if (entry_size > HASHMAP_ENTRY_MAX_STACK_SIZE) {
        HashmapEntry* entry = malloc(entry_size);
        assert(entry != NULL);

        entry->hash = computed_hash;

        void* entry_key = hashmap_entry_key(map, entry);
        if (key == NULL) {
            memset(entry_key, 0, map->key_specs.size);
        }
        else {
            memcpy(entry_key, key, map->key_specs.size);
        }

        void* entry_value = hashmap_entry_value(map, entry);
        if (value == NULL) {
            memset(entry_value, 0, map->value_specs.size);
        }
        else {
            memcpy(entry_value, value, map->value_specs.size);
        }

        vector_push_back(bucket, &entry);
    }
    else {
        u8 stack_entry_buf[HASHMAP_ENTRY_MAX_STACK_SIZE] = { 0 };
        HashmapEntry* entry = (HashmapEntry*)&stack_entry_buf;

        entry->hash = computed_hash;

        void* entry_key = hashmap_entry_key(map, entry);
        if (key == NULL) {
            memset(entry_key, 0, map->key_specs.size);
        }
        else {
            memcpy(entry_key, key, map->key_specs.size);
        }

        void* entry_value = hashmap_entry_value(map, entry);
        if (value == NULL) {
            memset(entry_value, 0, map->value_specs.size);
        }
        else {
            memcpy(entry_value, value, map->value_specs.size);
        }

        vector_push_back(bucket, entry);
    }

    map->size++;
}

// -- retrieval --

bool hashmap_contains(const Hashmap* map, const void* key) {
    assert(map != NULL);
    assert(map->key_specs.is_ptr || (key != NULL && "key can be NULL only if item type is ptr"));
    return hashmap_find_entry(map, key) != NULL;
}

void* hashmap_get(const Hashmap* map, const void* key) {
    assert(map != NULL);
    assert(map->key_specs.is_ptr || (key != NULL && "key can be NULL only if item type is ptr"));

    HashmapEntry* entry = hashmap_find_entry(map, key);
    return (entry == NULL)
        ? NULL
        : ITEM_SPEC_CAST(map->value_specs.is_ptr, hashmap_entry_value(map, entry));
}

const void* hashmap_get_key(const Hashmap* map, const void* key) {
    assert(map != NULL);
    assert(map->key_specs.is_ptr || (key != NULL && "key can be NULL only if item type is ptr"));

    HashmapEntry* entry = hashmap_find_entry(map, key);
    return (entry == NULL)
        ? NULL
        : ITEM_SPEC_CAST(map->key_specs.is_ptr, hashmap_entry_key(map, entry));
}

// -- removal --

bool hashmap_remove(Hashmap* map, const void* key) {
    assert(map != NULL);
    assert(map->key_specs.is_ptr || (key != NULL && "key can be NULL only if item type is ptr"));

    const void* actual_key = ITEM_SPEC_CAST(map->key_specs.is_ptr, key);
    const u32 computed_hash = map->key_specs.hash_fn(actual_key);
    const u32 bucket_idx = computed_hash % map->capacity;

    HashmapBucket* bucket = &map->buckets[bucket_idx];
    if (bucket->raw == NULL) {
        return false;
    }

    for (u32 i = 0; i < bucket->size; i++) {
        HashmapEntry* entry = vector_at(*bucket, i);
        if (computed_hash != entry->hash) {
            continue;
        }

        void* entry_key = hashmap_entry_key(map, entry);
        void* actual_entry_key = ITEM_SPEC_CAST(map->key_specs.is_ptr, entry_key);

        if (map->key_specs.equals_fn(actual_key, actual_entry_key)) {
            hashmap_entry_destroy(map, entry);
            vector_remove(bucket, i);

            map->size--;
            return true;
        }
    }
    return false;
}

// -- iteration --

HashmapIterator hashmap_get_it(const Hashmap* map) {
    assert(map != NULL);
    return (HashmapIterator) {
        .map = map,
            .bucket_index = 0,
            .item_index = 0,
    };
}

bool hashmap_it_next(HashmapIterator* it, const void* key, void* value) {
    assert(it != NULL && it->map != NULL);

    while (it->bucket_index < it->map->capacity) {
        HashmapBucket* bucket = &it->map->buckets[it->bucket_index];

        if (bucket->raw != NULL && it->item_index < bucket->size) {
            HashmapEntry* entry = vector_at(*bucket, it->item_index);

            if (key != NULL) {
                key = ITEM_SPEC_CAST(it->map->key_specs.is_ptr, hashmap_entry_key(it->map, entry));
            }
            if (value != NULL) {
                value = ITEM_SPEC_CAST(it->map->value_specs.is_ptr, hashmap_entry_value(it->map, entry));
            }

            it->item_index++;
            return true;
        }

        it->bucket_index++;
        it->item_index = 0;
    }

    return false;
}