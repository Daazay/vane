#include <utest/utest.h>

#include <vane/utils/vector.h>

#include "test_object.h"

struct TestVectorStruct {
    Vector vec;
};

UTEST_F_SETUP(TestVectorStruct) {
    utest_fixture->vec = vector_create(0, VECTOR_ITEM_SPECS(TestObject, NULL));
}

UTEST_F_TEARDOWN(TestVectorStruct) {
    vector_destroy(&utest_fixture->vec);
}

// -- Insertion --

UTEST_F(TestVectorStruct, vector_push_front1) {
    TestObject value = test_object_create(1, 16, 10);
    vector_push_front(&utest_fixture->vec, &value);

    ASSERT_TRUE(test_object_eq(&value, vector_at(&utest_fixture->vec, 0)));
}

UTEST_F(TestVectorStruct, vector_push_back1) {
    TestObject value = test_object_create(1, 16, 10);
    vector_push_back(&utest_fixture->vec, &value);

    ASSERT_TRUE(test_object_eq(&value, vector_at(&utest_fixture->vec, 0)));
}

UTEST_F(TestVectorStruct, vector_push_front2) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    for (u32 i = 0; i < values_size; ++i) {
        vector_push_front(&utest_fixture->vec, &values[i]);
    }

    for (u32 i = 0; i < utest_fixture->vec.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[values_size - i - 1], vector_at(&utest_fixture->vec, i)));
    }
}

UTEST_F(TestVectorStruct, vector_push_back2) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    for (u32 i = 0; i < values_size; ++i) {
        vector_push_back(&utest_fixture->vec, &values[i]);
    }

    for (u32 i = 0; i < utest_fixture->vec.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(&utest_fixture->vec, i)));
    }
}

UTEST_F(TestVectorStruct, vector_insert1) {
    TestObject value = test_object_create(1, 16, 10);
    vector_insert(&utest_fixture->vec, 0, &value, 1);

    ASSERT_TRUE(test_object_eq(&value, vector_at(&utest_fixture->vec, 0)));
}

UTEST_F(TestVectorStruct, vector_insert2) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    vector_insert(&utest_fixture->vec, 0, &values, values_size);

    for (u32 i = 0; i < utest_fixture->vec.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(&utest_fixture->vec, i)));
    }
}

UTEST_F(TestVectorStruct, vector_insert3) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    vector_insert(&utest_fixture->vec, 0, &values, values_size);

    TestObject value = test_object_create(1, 16, 10);
    vector_insert(&utest_fixture->vec, 0, &value, 1);

    ASSERT_TRUE(test_object_eq(&value, vector_at(&utest_fixture->vec, 0)));
    for (u32 i = 1; i < utest_fixture->vec.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i - 1], vector_at(&utest_fixture->vec, i)));
    }
}

UTEST_F(TestVectorStruct, vector_insert4) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    vector_insert(&utest_fixture->vec, 0, &values, values_size);

    TestObject value = test_object_create(1, 16, 10);
    vector_insert(&utest_fixture->vec, values_size, &value, 1);

    for (u32 i = 0; i < values_size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(&utest_fixture->vec, i)));
    }
    ASSERT_TRUE(test_object_eq(&value, vector_at(&utest_fixture->vec, values_size)));
}

UTEST_F(TestVectorStruct, vector_insert5) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    vector_insert(&utest_fixture->vec, 0, &values, values_size);

    TestObject value = test_object_create(1, 16, 10);
    vector_insert(&utest_fixture->vec, 2, &value, 1);

    ASSERT_TRUE(test_object_eq(&value, vector_at(&utest_fixture->vec, 2)));
    for (u32 i = 0; i < 2; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(&utest_fixture->vec, i)));
    }
    for (u32 i = 3; i < utest_fixture->vec.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i - 1], vector_at(&utest_fixture->vec, i)));
    }
}

UTEST_F(TestVectorStruct, vector_extend1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    vector_extend(&utest_fixture->vec, values, values_size);

    for (u32 i = 0; i < utest_fixture->vec.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(&utest_fixture->vec, i)));
    }
}

UTEST_F(TestVectorStruct, vector_extend2) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };

    vector_extend(&utest_fixture->vec, values, 2);
    vector_extend(&utest_fixture->vec, values + 2, 3);

    for (u32 i = 0; i < utest_fixture->vec.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(&utest_fixture->vec, i)));
    }
}

// -- Removal --

UTEST_F(TestVectorStruct, vector_remove1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend(&utest_fixture->vec, values, ARR_SIZE(values));

    vector_remove(&utest_fixture->vec, 0);

    ASSERT_EQ(2, utest_fixture->vec.size);
    ASSERT_TRUE(test_object_eq(&values[1], vector_at(&utest_fixture->vec, 0)));
    ASSERT_TRUE(test_object_eq(&values[2], vector_at(&utest_fixture->vec, 1)));
}

UTEST_F(TestVectorStruct, vector_remove2) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend(&utest_fixture->vec, values, ARR_SIZE(values));

    vector_remove(&utest_fixture->vec, 2);

    ASSERT_EQ(2, utest_fixture->vec.size);
    ASSERT_TRUE(test_object_eq(&values[0], vector_at(&utest_fixture->vec, 0)));
    ASSERT_TRUE(test_object_eq(&values[1], vector_at(&utest_fixture->vec, 1)));
}

UTEST_F(TestVectorStruct, vector_remove3) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend(&utest_fixture->vec, values, ARR_SIZE(values));

    vector_remove(&utest_fixture->vec, 1);

    ASSERT_EQ(2, utest_fixture->vec.size);
    ASSERT_TRUE(test_object_eq(&values[0], vector_at(&utest_fixture->vec, 0)));
    ASSERT_TRUE(test_object_eq(&values[2], vector_at(&utest_fixture->vec, 1)));
}

UTEST_F(TestVectorStruct, vector_pop_front1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend(&utest_fixture->vec, values, ARR_SIZE(values));

    vector_pop_front(&utest_fixture->vec);

    ASSERT_EQ(2, utest_fixture->vec.size);
    ASSERT_TRUE(test_object_eq(&values[1], vector_at_front(&utest_fixture->vec)));
}

UTEST_F(TestVectorStruct, vector_pop_back1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend(&utest_fixture->vec, values, ARR_SIZE(values));

    vector_pop_back(&utest_fixture->vec);

    ASSERT_EQ(2, utest_fixture->vec.size);
    ASSERT_TRUE(test_object_eq(&values[1], vector_at_back(&utest_fixture->vec)));
}

// -- Modification --

UTEST_F(TestVectorStruct, vector_set1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };
    vector_extend(&utest_fixture->vec, values, ARR_SIZE(values));

    TestObject value = test_object_create(1, 16, 10);
    vector_set(&utest_fixture->vec, 1, &value);
    ASSERT_TRUE(test_object_eq(&value, vector_at(&utest_fixture->vec, 1)));
}