#include "vane/utils/hashset.h"

#include <stdlib.h>
#include <string.h>

#include "vane/utils/vector.h"

typedef struct HashsetEntry HashsetEntry;
struct HashsetEntry {
    u32 hash;
    void* data;
};

static inline u64 hashset_entry_size(const Hashset* set) {
    return sizeof(u32) + set->item_specs.size;
}

static inline void* hashset_entry_item(const HashsetEntry* entry) {
    return (u8*)entry + sizeof(u32);
}

static inline void hashset_entry_destroy(const Hashset* set, HashsetEntry* entry) {
    if (set->item_specs.destroy_fn != NULL) {
        void* item = hashset_entry_item(entry);
        void* actual_item = ITEM_SPECS_CAST(set->item_specs, item);
        set->item_specs.destroy_fn(actual_item);
    }
}

static inline void hashset_bucket_init(const Hashset* set, HashsetBucket* bucket) {
    const u64 entry_size = hashset_entry_size(set);
    const bool is_entry_big = entry_size > HASHSET_ENTRY_MAX_STACK_SIZE;

    *bucket = vector_create(HASHSET_INIT_BUCKET_SIZE, (VectorItemSpecs) {
        .size = (u32)(is_entry_big ? sizeof(HashsetEntry*) : entry_size),
        .is_ptr = is_entry_big,
        .destroy_fn = is_entry_big ? &free : NULL,
    });
}

static inline HashsetEntry* hashset_find_entry(const Hashset* set, const void* item) {
    const void* actual_item = ITEM_SPECS_CAST(set->item_specs, item);
    const u32 computed_hash = set->item_specs.hash_fn(actual_item);
    const u32 bucket_idx = computed_hash % set->capacity;

    const HashsetBucket* bucket = &set->buckets[bucket_idx];

    if (bucket->raw == NULL) {
        return NULL;
    }

    for (u32 i = 0; i < bucket->size; i++) {
        HashsetEntry* entry = vector_at(bucket, i);

        if (computed_hash == entry->hash) {
            void* entry_item = hashset_entry_item(entry);
            void* actual_entry_item = ITEM_SPECS_CAST(set->item_specs, entry_item);
            if (set->item_specs.equals_fn(actual_item, actual_entry_item)) {
                return entry;
            }
        }
    }

    return NULL;
}

// -- creatrion --

Hashset hashset_create(u32 init_cap, HashsetItemSpecs item_specs) {
    assert(item_specs.size > 0 && item_specs.hash_fn != NULL && item_specs.equals_fn != NULL);

    u32 cap = (init_cap > 2)
        ? init_cap
        : HASHSET_DEFAULT_CAPACITY;

    HashsetBucket* buckets = calloc(cap, sizeof(HashsetBucket));
    assert(buckets != NULL);

    return (Hashset) {
        .item_specs = item_specs,
        .buckets = buckets,
        .capacity = cap,
        .size = 0,
    };
}

// -- destruction --

void hashset_destroy(Hashset* set) {
    if (set == NULL || set->buckets == NULL) {
        return;
    }

    hashset_clear(set);

    free(set->buckets);

    set->buckets = NULL;
    set->size = 0;
    set->capacity = 0;
}

void hashset_clear(Hashset* set) {
    assert(set != NULL);

    for (u32 i = 0; i < set->capacity; i++) {
        HashsetBucket* bucket = &set->buckets[i];
        if (bucket->raw == NULL) {
            continue;
        }

        for (u32 j = 0; j < bucket->size; j++) {
            HashsetEntry* entry = vector_at(bucket, j);
            hashset_entry_destroy(set, entry);
        }
        vector_destroy(bucket);
    }

    set->size = 0;
}

// -- utility --

bool is_hashset_empty(const Hashset* set) {
    assert(set != NULL);
    return set->size == 0;
}

void hashset_rehash(Hashset* set, u32 new_cap) {
    assert(set != NULL && new_cap > 0);

    if (new_cap == set->capacity) {
        return;
    }

    HashsetBucket* new_buckets = calloc(new_cap, sizeof(HashsetBucket));
    assert(new_buckets != NULL);

    for (u32 i = 0; i < set->capacity; i++) {
        HashsetBucket* old_bucket = &set->buckets[i];
        if (old_bucket->raw == NULL) {
            continue;
        }

        for (u32 j = 0; j < old_bucket->size; j++) {
            HashsetEntry* entry = vector_at(old_bucket, j);

            const u32 new_bucket_idx = entry->hash % new_cap;
            HashsetBucket* new_bucket = &new_buckets[new_bucket_idx];
            if (new_bucket->raw == NULL) {
                hashset_bucket_init(set, new_bucket);
            }

            vector_push_back(new_bucket, entry);
        }
        vector_destroy(old_bucket);
    }

    free(set->buckets);
    set->buckets = new_buckets;
    set->capacity = new_cap;
}

void hashset_shrink_to_fit(Hashset* set) {
    assert(set != NULL);

    if (set->size == 0) {
        if (set->capacity > HASHSET_DEFAULT_CAPACITY) {
            hashset_rehash(set, HASHSET_DEFAULT_CAPACITY);
        }
        return;
    }

    // Find the minimal capacity such that size <= cap * load_factor
    u32 min_cap = HASHSET_DEFAULT_CAPACITY;
    while ((u32)(min_cap * HASHSET_LOAD_FACTOR) < set->size) {
        min_cap *= HASHSET_GROWTH_FACTOR;
    }

    if (min_cap < set->capacity) {
        hashset_rehash(set, min_cap);
    }
}

// -- insertion --

void hashset_insert(Hashset* set, const void* item) {
    assert(set != NULL);
    assert(set->item_specs.is_ptr || item != NULL && "item can be NULL only if item type is ptr");

    HashsetEntry* existing_entry = hashset_find_entry(set, item);
    if (existing_entry != NULL) {
        // item already exists, do nothing
        return;
    }

    if (set->size >= set->capacity * HASHSET_LOAD_FACTOR) {
        hashset_rehash(set, set->capacity * HASHSET_GROWTH_FACTOR);
    }

    const void* actual_item = ITEM_SPECS_CAST(set->item_specs, item);
    const u32 computed_hash = set->item_specs.hash_fn(actual_item);
    const u32 bucket_idx = computed_hash % set->capacity;

    HashsetBucket* bucket = &set->buckets[bucket_idx];
    if (bucket->raw == NULL) {
        hashset_bucket_init(set, bucket);
    }

    const u64 entry_size = hashset_entry_size(set);

    if (entry_size > HASHSET_ENTRY_MAX_STACK_SIZE) {
        HashsetEntry* entry = malloc(entry_size);
        assert(entry != NULL);

        entry->hash = computed_hash;
        void* entry_item = hashset_entry_item(entry);

        if (item == NULL) {
            memset(entry_item, 0, set->item_specs.size);
        }
        else {
            memcpy(entry_item, item, set->item_specs.size);
        }

        vector_push_back(bucket, &entry);
    }
    else {
        u8 stack_entry_buf[HASHSET_ENTRY_MAX_STACK_SIZE] = { 0 };
        HashsetEntry* entry = (HashsetEntry*)&stack_entry_buf;

        entry->hash = computed_hash;
        void* entry_item = hashset_entry_item(entry);

        if (item == NULL) {
            memset(entry_item, 0, set->item_specs.size);
        }
        else {
            memcpy(entry_item, item, set->item_specs.size);
        }

        vector_push_back(bucket, entry);
    }

    set->size++;
}

// -- retrieval --

bool hashset_contains(const Hashset* set, const void* item) {
    assert(set != NULL);
    assert(set->item_specs.is_ptr || item != NULL && "item can be NULL only if item type is ptr");
    return hashset_find_entry(set, item) != NULL;
}

const void* hashset_get(const Hashset* set, const void* item) {
    assert(set != NULL);
    assert(set->item_specs.is_ptr || item != NULL && "item can be NULL only if item type is ptr");

    HashsetEntry* entry = hashset_find_entry(set, item);
    return (entry == NULL)
        ? NULL
        : ITEM_SPECS_CAST(set->item_specs, hashset_entry_item(entry));
}

// -- removal --

bool hashset_remove(Hashset* set, const void* item) {
    assert(set != NULL);
    assert(set->item_specs.is_ptr || item != NULL && "item can be NULL only if item type is ptr");

    const void* actual_item = ITEM_SPECS_CAST(set->item_specs, item);
    const u32 computed_hash = set->item_specs.hash_fn(actual_item);
    const u32 bucket_idx = computed_hash % set->capacity;

    HashsetBucket* bucket = &set->buckets[bucket_idx];
    if (bucket->raw == NULL) {
        return false;
    }

    for (u32 i = 0; i < bucket->size; i++) {
        HashsetEntry* entry = vector_at(bucket, i);

        if (computed_hash != entry->hash) {
            continue;
        }

        const void* entry_item = hashset_entry_item(entry);
        const void* actual_entry_item = ITEM_SPECS_CAST(set->item_specs, entry_item);

        if (set->item_specs.equals_fn(actual_item, actual_entry_item)) {
            hashset_entry_destroy(set, entry);
            vector_remove(bucket, i);

            set->size--;
            return true;
        }
    }
    return false;
}

// -- iteration --

HashsetIterator hashset_get_it(const Hashset* set) {
    assert(set != NULL);
    return (HashsetIterator) {
        .set = set,
        .bucket_index = 0,
        .item_index = 0,
        .item = NULL,
    };
}

bool hashset_it_next(HashsetIterator* it) {
    assert(it != NULL && it->set != NULL);

    while (it->bucket_index < it->set->capacity) {
        const HashsetBucket* bucket = &it->set->buckets[it->bucket_index];

        if (bucket->raw != NULL && it->item_index < bucket->size) {
            HashsetEntry* entry = vector_at(bucket, it->item_index);
            it->item = ITEM_SPECS_CAST(it->set->item_specs, hashset_entry_item(entry));
            it->item_index++;
            return true;
        }
        // Move to the next bucket
        it->bucket_index++;
        it->item_index = 0;
    }
    return false;
}