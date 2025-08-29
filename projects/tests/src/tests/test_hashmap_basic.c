#include <utest/utest.h>

#include <vane/utils/hashmap.h>

typedef struct HashmapBasicPair HashmapBasicPair;

struct HashmapBasicPair {
    u32 key;
    i64 value;
};

static u32 u32_hash(const void* data) {
    return *(const u32*)data;
}

static bool u32_equals(const void* a, const void* b) {
    return *(const u32*)a == *(const u32*)b;
}

struct TestHashmapBasic {
    Hashmap map;
};

#define MAP utest_fixture->map

UTEST_F_SETUP(TestHashmapBasic) {
    MAP = hashmap_create(0,
        HASHMAP_KEY_SPECS(u32, &u32_hash, &u32_equals, NULL),
        HASHMAP_VALUE_SPECS(i64, NULL)
    );
}

UTEST_F_TEARDOWN(TestHashmapBasic) {
    hashmap_destroy(&MAP);
}

UTEST_F(TestHashmapBasic, hashmap_insert1) {
    HashmapBasicPair pair = { 45, -21422413 };

    hashmap_insert(&MAP, &pair.key, &pair.value);

    ASSERT_EQ(1, MAP.size);
    ASSERT_EQ(pair.value, *(i64*)hashmap_get(&MAP, &pair.key));
}

UTEST_F(TestHashmapBasic, hashmap_insert2) {
    const HashmapBasicPair pairs[] = {
        { 45, -21422413 },
        { 111, 1255 },
        { 1255, 13 },
        { 2467, 1 },
        { 57, 900 },
    };
    const u32 pairs_count = sizeof(pairs) / sizeof(HashmapBasicPair);

    for (u32 i = 0; i < pairs_count; ++i) {
        hashmap_insert(&MAP, &pairs[i].key, &pairs[i].value);
    }

    ASSERT_EQ(pairs_count, MAP.size);

    for (u32 i = 0; i < pairs_count; ++i) {
        ASSERT_EQ(pairs[i].value, *(i64*)hashmap_get(&MAP, &pairs[i].key));
    }
}

UTEST_F(TestHashmapBasic, hashmap_contains1) {
    const HashmapBasicPair pairs[] = {
        { 45, -21422413 },
        { 111, 1255 },
        { 1255, 13 },
        { 2467, 1 },
        { 57, 900 },
    };
    const u32 pairs_count = sizeof(pairs) / sizeof(HashmapBasicPair);

    for (u32 i = 0; i < pairs_count; ++i) {
        hashmap_insert(&MAP, &pairs[i].key, &pairs[i].value);
    }

    const u32 existing_key = 111;
    ASSERT_TRUE(hashmap_contains(&MAP, &existing_key));

    const u32 not_existing_key = 124567;
    ASSERT_FALSE(hashmap_contains(&MAP, &not_existing_key));
}

UTEST_F(TestHashmapBasic, hashmap_it_next1) {
    HashmapIterator it = hashmap_get_it(&MAP);

    ASSERT_FALSE(hashmap_it_next(&it, NULL, NULL));
}

UTEST_F(TestHashmapBasic, hashmap_it_next2) {
    const HashmapBasicPair pairs[] = {
        { 45, -21422413 },
        { 111, 1255 },
        { 1255, 13 },
        { 2467, 1 },
        { 57, 900 },
    };
    const u32 pairs_count = sizeof(pairs) / sizeof(HashmapBasicPair);

    for (u32 i = 0; i < pairs_count; ++i) {
        hashmap_insert(&MAP, &pairs[i].key, &pairs[i].value);
    }

    HashmapIterator it = hashmap_get_it(&MAP);
    for (u32 i = 0; i < pairs_count; ++i) {
        ASSERT_TRUE(hashmap_it_next(&it, NULL, NULL));
    }
    ASSERT_FALSE(hashmap_it_next(&it, NULL, NULL));
}