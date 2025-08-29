#include <utest/utest.h>

#include <vane/utils/hashmap.h>

#include "test_object.h"

typedef struct HashmapStructPair HashmapStructPair;

struct HashmapStructPair {
    TestObject key;
    TestObject value;
};

struct TestHashmapStruct {
    Hashmap map;
};

#define MAP utest_fixture->map

UTEST_F_SETUP(TestHashmapStruct) {
    MAP = hashmap_create(0,
        HASHMAP_KEY_SPECS(TestObject, &get_test_object_hash, &test_object_eq, NULL),
        HASHMAP_VALUE_SPECS(TestObject, NULL)
    );
}

UTEST_F_TEARDOWN(TestHashmapStruct) {
    hashmap_destroy(&MAP);
}

UTEST_F(TestHashmapStruct, hashmap_insert1) {
    HashmapStructPair pair = { test_object_create(12, 33, 1),  test_object_create(1489, 65, 14) };

    hashmap_insert(&MAP, &pair.key, &pair.value);

    ASSERT_EQ(1, MAP.size);
    ASSERT_TRUE(test_object_eq(&pair.value, hashmap_get(&MAP, &pair.key)));
}

UTEST_F(TestHashmapStruct, hashmap_insert2) {
    const HashmapStructPair pairs[] = {
        { test_object_create(12, 33, 1),  test_object_create(1489, 65, 14) },
        { test_object_create(75, 15, 3),   test_object_create(612, 73, 13) },
        { test_object_create(15, 73, 6),   test_object_create(6251, 0, 7) },
        { test_object_create(97, 1, 24),  test_object_create(2677, 67, 9) },
        { test_object_create(34, 15, 3),   test_object_create(7899, 73, 17) },
        { test_object_create(77, 0, 24), test_object_create(11, 15, 0) },
    };
    const u32 pairs_count = sizeof(pairs) / sizeof(HashmapStructPair);

    for (u32 i = 0; i < pairs_count; ++i) {
        hashmap_insert(&MAP, &pairs[i].key, &pairs[i].value);
    }

    ASSERT_EQ(pairs_count, MAP.size);

    for (u32 i = 0; i < pairs_count; ++i) {
        ASSERT_TRUE(test_object_eq(&pairs[i].value, hashmap_get(&MAP, &pairs[i].key)));
    }
}

UTEST_F(TestHashmapStruct, hashmap_contains1) {
    const HashmapStructPair pairs[] = {
        { test_object_create(12, 33, 1),  test_object_create(1489, 65, 14) },
        { test_object_create(75, 15, 3),   test_object_create(612, 73, 13) },
        { test_object_create(15, 73, 6),   test_object_create(6251, 0, 7) },
        { test_object_create(97, 1, 24),  test_object_create(2677, 67, 9) },
        { test_object_create(34, 15, 3),   test_object_create(7899, 73, 17) },
        { test_object_create(77, 0, 24), test_object_create(11, 15, 0) },
    };
    const u32 pairs_count = sizeof(pairs) / sizeof(HashmapStructPair);

    for (u32 i = 0; i < pairs_count; ++i) {
        hashmap_insert(&MAP, &pairs[i].key, &pairs[i].value);
    }

    const TestObject existing_key = test_object_create(34, 15, 3);
    ASSERT_TRUE(hashmap_contains(&MAP, &existing_key));

    const TestObject not_existing_key = test_object_create(134, 45, 15);
    ASSERT_FALSE(hashmap_contains(&MAP, &not_existing_key));
}

UTEST_F(TestHashmapStruct, hashmap_it_next1) {
    HashmapIterator it = hashmap_get_it(&MAP);

    ASSERT_FALSE(hashmap_it_next(&it, NULL, NULL));
}

UTEST_F(TestHashmapStruct, hashmap_it_next2) {
    const HashmapStructPair pairs[] = {
        { test_object_create(12, 33, 1),  test_object_create(1489, 65, 14) },
        { test_object_create(75, 15, 3),   test_object_create(612, 73, 13) },
        { test_object_create(15, 73, 6),   test_object_create(6251, 0, 7) },
        { test_object_create(97, 1, 24),  test_object_create(2677, 67, 9) },
        { test_object_create(34, 15, 3),   test_object_create(7899, 73, 17) },
        { test_object_create(77, 0, 24), test_object_create(11, 15, 0) },
    };
    const u32 pairs_count = sizeof(pairs) / sizeof(HashmapStructPair);

    for (u32 i = 0; i < pairs_count; ++i) {
        hashmap_insert(&MAP, &pairs[i].key, &pairs[i].value);
    }

    HashmapIterator it = hashmap_get_it(&MAP);
    for (u32 i = 0; i < pairs_count; ++i) {
        ASSERT_TRUE(hashmap_it_next(&it, NULL, NULL));
    }
    ASSERT_FALSE(hashmap_it_next(&it, NULL, NULL));
}
