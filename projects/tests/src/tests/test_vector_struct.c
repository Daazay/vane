#include <utest/utest.h>

#include <vane/utils/vector.h>

#include "../utils/test_object.h"

struct TestVectorStruct {
    Vector vec;
};

#define VEC utest_fixture->vec

UTEST_F_SETUP(TestVectorStruct) {
    VEC = vector_create(0, VECTOR_SPECS(TestObject, NULL));
}

UTEST_F_TEARDOWN(TestVectorStruct) {
    vector_destroy(&VEC);
}

// -- Insertion --

UTEST_F(TestVectorStruct, vector_push_front1) {
    TestObject value = test_object_create(1, 16, 10);
    vector_push_front(&VEC, &value);

    ASSERT_TRUE(test_object_eq(&value, vector_at(VEC, 0)));
}

UTEST_F(TestVectorStruct, vector_push_back1) {
    TestObject value = test_object_create(1, 16, 10);
    vector_push_back(&VEC, &value);

    ASSERT_TRUE(test_object_eq(&value, vector_at(VEC, 0)));
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
        vector_push_front(&VEC, &values[i]);
    }

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[values_size - i - 1], vector_at(VEC, i)));
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
        vector_push_back(&VEC, &values[i]);
    }

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(VEC, i)));
    }
}

UTEST_F(TestVectorStruct, vector_insert1) {
    TestObject value = test_object_create(1, 16, 10);
    vector_insert(&VEC, 0, &value, 1);

    ASSERT_TRUE(test_object_eq(&value, vector_at(VEC, 0)));
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

    vector_insert(&VEC, 0, &values, values_size);

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(VEC, i)));
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

    vector_insert(&VEC, 0, &values, values_size);

    TestObject value = test_object_create(1, 16, 10);
    vector_insert(&VEC, 0, &value, 1);

    ASSERT_TRUE(test_object_eq(&value, vector_at(VEC, 0)));
    for (u32 i = 1; i < VEC.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i - 1], vector_at(VEC, i)));
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

    vector_insert(&VEC, 0, &values, values_size);

    TestObject value = test_object_create(1, 16, 10);
    vector_insert(&VEC, values_size, &value, 1);

    for (u32 i = 0; i < values_size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(VEC, i)));
    }
    ASSERT_TRUE(test_object_eq(&value, vector_at(VEC, values_size)));
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

    vector_insert(&VEC, 0, &values, values_size);

    TestObject value = test_object_create(1, 16, 10);
    vector_insert(&VEC, 2, &value, 1);

    ASSERT_TRUE(test_object_eq(&value, vector_at(VEC, 2)));
    for (u32 i = 0; i < 2; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(VEC, i)));
    }
    for (u32 i = 3; i < VEC.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i - 1], vector_at(VEC, i)));
    }
}

UTEST_F(TestVectorStruct, vector_extend_back1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    vector_extend_back(&VEC, values, values_size);

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(VEC, i)));
    }
}

UTEST_F(TestVectorStruct, vector_extend_back2) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };

    vector_extend_back(&VEC, values, 2);
    vector_extend_back(&VEC, values + 2, 3);

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(VEC, i)));
    }
}

UTEST_F(TestVectorStruct, vector_extend_front1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };
    const u32 values_size = ARR_SIZE(values);

    vector_extend_front(&VEC, values, values_size);

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(VEC, i)));
    }
}

UTEST_F(TestVectorStruct, vector_extend_front2) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
        test_object_create(4, 44, 40),
        test_object_create(5, 55, 50),
    };

    vector_extend_front(&VEC, values, 2);
    vector_extend_front(&VEC, values + 2, 3);

    for (u32 i = 0; i < ARR_SIZE(values) - 2; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i + 2], vector_at(VEC, i)));
    }

    for (u32 i = 0; i < 2; ++i) {
        ASSERT_TRUE(test_object_eq(&values[i], vector_at(VEC, i + ARR_SIZE(values) - 2)));
    }
}

// -- Removal --

UTEST_F(TestVectorStruct, vector_remove1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_remove(&VEC, 0);

    ASSERT_EQ(2, VEC.size);
    ASSERT_TRUE(test_object_eq(&values[1], vector_at(VEC, 0)));
    ASSERT_TRUE(test_object_eq(&values[2], vector_at(VEC, 1)));
}

UTEST_F(TestVectorStruct, vector_remove2) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_remove(&VEC, 2);

    ASSERT_EQ(2, VEC.size);
    ASSERT_TRUE(test_object_eq(&values[0], vector_at(VEC, 0)));
    ASSERT_TRUE(test_object_eq(&values[1], vector_at(VEC, 1)));
}

UTEST_F(TestVectorStruct, vector_remove3) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_remove(&VEC, 1);

    ASSERT_EQ(2, VEC.size);
    ASSERT_TRUE(test_object_eq(&values[0], vector_at(VEC, 0)));
    ASSERT_TRUE(test_object_eq(&values[2], vector_at(VEC, 1)));
}

UTEST_F(TestVectorStruct, vector_pop_front1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_pop_front(&VEC);

    ASSERT_EQ(2, VEC.size);
    ASSERT_TRUE(test_object_eq(&values[1], vector_at_front(VEC)));
}

UTEST_F(TestVectorStruct, vector_pop_back1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_pop_back(&VEC);

    ASSERT_EQ(2, VEC.size);
    ASSERT_TRUE(test_object_eq(&values[1], vector_at_back(VEC)));
}

// -- Modification --

UTEST_F(TestVectorStruct, vector_set1) {
    TestObject values[] = {
        test_object_create(1, 11, 10),
        test_object_create(2, 22, 20),
        test_object_create(3, 33, 30),
    };
    vector_extend_back(&VEC, values, ARR_SIZE(values));

    TestObject value = test_object_create(1, 16, 10);
    vector_set(VEC, 1, &value);
    ASSERT_TRUE(test_object_eq(&value, vector_at(VEC, 1)));
}