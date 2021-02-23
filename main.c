#include <stdio.h>
#include "my_malloc.h"

int main() {
    printf("first malloc\n");
    void *x = my_malloc(8);
    printf("%p\n", x);
    printf("second malloc\n");
    void *y = my_malloc(8);
    printf("%p\n", y);
    printf("third malloc\n");
    void *z = my_malloc(8);
    printf("%p\n", z);
    my_free(z);
    my_free(y);
    printf("fourth malloc\n");
    void *w = my_calloc(1024, 1);
    printf("%p\n", w);
    x = my_malloc(8);
    printf("x %p\n", x);
}
