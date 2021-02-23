#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sys/mman.h>
#include <stdint.h>

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);
