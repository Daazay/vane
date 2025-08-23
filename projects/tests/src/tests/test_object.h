#pragma once

#include <stdlib.h>

#include <vane/utils/defines.h>

typedef struct TestObject TestObject;

struct TestObject {
    u32 a;
    u16 b;
    u64 c;
};

static TestObject test_object_create(u32 a, u16 b, u64 c) {
    return (TestObject) {
        .a = a,
        .b = b,
        .c = c,
    };
}

static TestObject* test_object_allocate(u32 a, u16 b, u64 c) {
    TestObject* obj = malloc(sizeof(TestObject));
    assert(obj != NULL);

    obj->a = a;
    obj->b = b;
    obj->c = c;

    return obj;
}

static void test_object_destroy(TestObject* obj) {
    if (obj == NULL) {
        return;
    }

    free(obj);
}

static i32 test_object_cmp(const TestObject* obj1, const TestObject* obj2) {
    assert(obj1 != NULL && obj2 != NULL);

    i64 sum1 = (obj1->a + obj1->b + obj1->c);
    i64 sum2 = (obj2->a + obj2->b + obj2->c);

    i64 diff = sum1 - sum2;
    return (diff > 0) - (diff < 0);
}

static i32 test_object_eq(const TestObject* obj1, const TestObject* obj2) {
    return test_object_cmp(obj1, obj2) == 0;
}

static u32 get_test_object_hash(const TestObject* obj) {
    return (u32)(obj->a + obj->b);
}