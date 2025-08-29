#include <utest/utest.h>

#include <vane/utils/hashset.h>

static u32 u32_hash(const void *data) {
    return *(const u32*)data;
}

static bool u32_equals(const void *a, const void *b) {
    return *(const u32*)a == *(const u32*)b;
}

struct TestHashsetBasic {
    Hashset set;
};

#define SET utest_fixture->set

UTEST_F_SETUP(TestHashsetBasic) {
    SET = hashset_create(0, HASHSET_ITEM_SPECS(u32, &u32_hash, &u32_equals, NULL));
}

UTEST_F_TEARDOWN(TestHashsetBasic) {
    hashset_destroy(&SET);
}

UTEST_F(TestHashsetBasic, hashset_insert1) {
    u32 value = 42;
    hashset_insert(&SET, &value);
    ASSERT_EQ(1, SET.size);
    ASSERT_TRUE(hashset_contains(&SET, &value));
}

UTEST_F(TestHashsetBasic, hashset_insert2) {
    const u32 items[] = { 45, 111, 1255, 2467, 57 };
    const u32 items_count = sizeof(items) / sizeof(u32);

    for (u32 i = 0; i < items_count; ++i) {
        hashset_insert(&SET, &items[i]);
    }

    ASSERT_EQ(items_count, SET.size);

    for (u32 i = 0; i < items_count; ++i) {
        ASSERT_TRUE(hashset_contains(&SET, &items[i]));
    }
}

UTEST_F(TestHashsetBasic, hashset_contains1) {
    const u32 items[] = { 45, 111, 1255, 2467, 57 };
    const u32 items_count = sizeof(items) / sizeof(u32);

    for (u32 i = 0; i < items_count; ++i) {
        hashset_insert(&SET, &items[i]);
    }

    const u32 existing_item = 111;
    ASSERT_TRUE(hashset_contains(&SET, &existing_item));

    const u32 not_existing_item = 124567;
    ASSERT_FALSE(hashset_contains(&SET, &not_existing_item));
}

UTEST_F(TestHashsetBasic, hashset_it_next1) {
    HashsetIterator it = hashset_get_it(&SET);

    ASSERT_FALSE(hashset_it_next(&it));
}

UTEST_F(TestHashsetBasic, hashset_it_next2) {
    const u32 items[] = { 45, 111, 1255, 2467, 57 };
    const u32 items_count = sizeof(items) / sizeof(u32);

    for (u32 i = 0; i < items_count; ++i) {
        hashset_insert(&SET, &items[i]);
    }

    HashsetIterator it = hashset_get_it(&SET);
    for (u32 i = 0; i < items_count; ++i) {
        ASSERT_TRUE(hashset_it_next(&it));
    }
    ASSERT_FALSE(hashset_it_next(&it));
}