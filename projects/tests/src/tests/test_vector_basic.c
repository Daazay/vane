#include <utest/utest.h>

#include <vane/utils/vector.h>

struct TestVectorBasic {
    Vector vec;
};

#define VEC utest_fixture->vec

UTEST_F_SETUP(TestVectorBasic) {
    VEC = vector_create(4, VECTOR_SPECS(u32, NULL));
}

UTEST_F_TEARDOWN(TestVectorBasic) {
    vector_destroy(&VEC);
}

// -- insertion --


UTEST_F(TestVectorBasic, vector_push_front1) {
    const u32 value = 5;
    vector_push_front(&VEC, &value);

    ASSERT_EQ(value, *(u32*)vector_at(VEC, 0));
}

UTEST_F(TestVectorBasic, vector_push_back1) {
    const u32 value = 5;
    vector_push_back(&VEC, &value);

    ASSERT_EQ(value, *(u32*)vector_at(VEC, 0));
}

UTEST_F(TestVectorBasic, vector_push_front2) {
    const u32 values[] = { 5, 12, 9, 22, 15 };
    const u32 values_size = ARR_SIZE(values);

    for (u32 i = 0; i < values_size; ++i) {
        vector_push_front(&VEC, &values[i]);
    }

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_EQ(values[values_size - i - 1], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_push_back2) {
    const u32 values[] = { 5, 12, 9, 22, 15 };
    const u32 values_size = ARR_SIZE(values);

    for (u32 i = 0; i < values_size; ++i) {
        vector_push_back(&VEC, &values[i]);
    }

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_EQ(values[i], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_insert1) {
    const u32 value = 5;
    vector_insert(&VEC, 0, &value, 1);

    ASSERT_EQ(value, *(u32*)vector_at(VEC, 0));
}

UTEST_F(TestVectorBasic, vector_insert2) {
    const u32 values[] = { 5, 12, 9, 22, 15 };
    const u32 values_size = ARR_SIZE(values);

    vector_insert(&VEC, 0, &values, values_size);

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_EQ(values[i], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_insert3) {
    const u32 values[] = { 5, 12, 9, 22, 15 };
    const u32 values_size = ARR_SIZE(values);

    vector_insert(&VEC, 0, &values, values_size);

    const u32 value = 17;
    vector_insert(&VEC, 0, &value, 1);

    ASSERT_EQ(value, *(u32*)vector_at(VEC, 0));
    for (u32 i = 1; i < VEC.size; ++i) {
        ASSERT_EQ(values[i - 1], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_insert4) {
    const u32 values[] = { 5, 12, 9, 22, 15 };
    const u32 values_size = ARR_SIZE(values);

    vector_insert(&VEC, 0, &values, values_size);

    const u32 value = 17;
    vector_insert(&VEC, values_size, &value, 1);

    ASSERT_EQ(value, *(u32*)vector_at(VEC, values_size));
    for (u32 i = 0; i < VEC.size - 1; ++i) {
        ASSERT_EQ(values[i], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_insert5) {
    const u32 values[] = { 5, 12, 9, 22, 15 };
    const u32 values_size = ARR_SIZE(values);

    vector_insert(&VEC, 0, &values, values_size);

    const u32 value = 17;
    vector_insert(&VEC, 2, &value, 1);

    ASSERT_EQ(value, *(u32*)vector_at(VEC, 2));
    for (u32 i = 0; i < 2; ++i) {
        ASSERT_EQ(values[i], *(u32*)vector_at(VEC, i));
    }
    for (u32 i = 3; i < VEC.size; ++i) {
        ASSERT_EQ(values[i - 1], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_extend_back1) {
    const u32 values[] = { 5, 12, 9, 22, 15 };
    const u32 values_size = ARR_SIZE(values);

    vector_extend_back(&VEC, values, values_size);

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_EQ(values[i], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_extend_back2) {
    const u32 values[] = { 5, 12, 9, 22, 15 };

    vector_extend_back(&VEC, values, 2);
    vector_extend_back(&VEC, values + 2, 3);

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_EQ(values[i], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_extend_front1) {
    const u32 values[] = { 5, 12, 9, 22, 15 };
    const u32 values_size = ARR_SIZE(values);

    vector_extend_front(&VEC, values, values_size);

    for (u32 i = 0; i < VEC.size; ++i) {
        ASSERT_EQ(values[i], *(u32*)vector_at(VEC, i));
    }
}

UTEST_F(TestVectorBasic, vector_extend_front2) {
    const u32 values[] = { 5, 12, 9, 22, 15 };

    vector_extend_front(&VEC, values, 2);
    vector_extend_front(&VEC, values + 2, 3);

    for (u32 i = 0; i < ARR_SIZE(values) - 2; ++i) {
        ASSERT_EQ(values[i + 2], *(u32*)vector_at(VEC, i));
    }

    for (u32 i = 0; i < 2; ++i) {
        ASSERT_EQ(values[i], *(u32*)vector_at(VEC, i + ARR_SIZE(values) - 2));
    }
}

// -- removal --

UTEST_F(TestVectorBasic, vector_remove1) {
    const u32 values[] = { 5, 12, 9 };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_remove(&VEC, 0);

    ASSERT_EQ(2, VEC.size);
    ASSERT_EQ(values[1], *(u32*)vector_at(VEC, 0));
    ASSERT_EQ(values[2], *(u32*)vector_at(VEC, 1));
}

UTEST_F(TestVectorBasic, vector_remove2) {
    const u32 values[] = { 5, 12, 9 };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_remove(&VEC, 2);

    ASSERT_EQ(2, VEC.size);
    ASSERT_EQ(values[0], *(u32*)vector_at(VEC, 0));
    ASSERT_EQ(values[1], *(u32*)vector_at(VEC, 1));
}

UTEST_F(TestVectorBasic, vector_remove3) {
    const u32 values[] = { 5, 12, 9 };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_remove(&VEC, 1);

    ASSERT_EQ(2, VEC.size);
    ASSERT_EQ(values[0], *(u32*)vector_at(VEC, 0));
    ASSERT_EQ(values[2], *(u32*)vector_at(VEC, 1));
}

UTEST_F(TestVectorBasic, vector_pop_front1) {
    const u32 values[] = { 5, 12, 9 };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_pop_front(&VEC);

    ASSERT_EQ(2, VEC.size);
    ASSERT_EQ(values[1], *(u32*)vector_at_front(VEC));
}

UTEST_F(TestVectorBasic, vector_pop_back1) {
    const u32 values[] = { 5, 12, 9 };

    vector_extend_back(&VEC, values, ARR_SIZE(values));

    vector_pop_back(&VEC);

    ASSERT_EQ(2, VEC.size);
    ASSERT_EQ(values[1], *(u32*)vector_at_back(VEC));
}

// -- modification --

UTEST_F(TestVectorBasic, vector_set1) {
    const u32 values[] = { 5, 12, 9 };
    vector_extend_back(&VEC, values, ARR_SIZE(values));

    const u32 new_val = 20;
    vector_set(VEC, 1, &new_val);
    ASSERT_EQ(new_val, *(u32*)vector_at(VEC, 1));
}