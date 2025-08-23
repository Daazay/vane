#include <stdio.h>
#include <stdlib.h>
#include <vane/vane.h>

int main(int argc, const char **argv) {
    (void)argc; (void)argv;
    printf("sum: %d + %d = %d\n", 5, 5, sum(5, 5));
    return 0;
}
