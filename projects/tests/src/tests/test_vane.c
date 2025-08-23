#include <utest/utest.h>

#include <vane/vane.h>

UTEST(TestSum, sum) {
    ASSERT_EQ(5, sum(3, 2));
}