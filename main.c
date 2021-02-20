#include <stdio.h>
#include "my_malloc.h"

int main() {
    void *x = my_malloc(8);
    void *y = my_malloc(8);
    void *z = my_malloc(8);
    printf("%p %p %p", x, y, z);
}
