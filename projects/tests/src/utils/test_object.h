#pragma once

#include <stdlib.h>

#include <vane/utils/defines.h>

typedef struct TestObject TestObject;

struct TestObject {
    u32 a;
    u16 b;
    u64 c;
};

TestObject test_object_create(u32 a, u16 b, u64 c);

TestObject* test_object_allocate(u32 a, u16 b, u64 c);

void test_object_destroy(TestObject* obj);

i32 test_object_cmp(const TestObject* obj1, const TestObject* obj2);

bool test_object_eq(const TestObject* obj1, const TestObject* obj2);

u32 get_test_object_hash(const TestObject* obj);