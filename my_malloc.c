#include "my_malloc.h"
#define NULL 0
#define _STARTING_MEMORY 128

static void *_my_mem = NULL;
static size_t allocated = _STARTING_MEMORY;

// every chunk contains the chunk size x, and then x-sizeof(size_t) bytes

// in my malloc, sizes are aligned to 4
// 0b01 (last bit) - represents whether this is free
// 0b10 (the one before last bit) - represents whether this is the last chunk

static inline void initialize_my_mem() {
    if (_my_mem == NULL) {
        _my_mem = mmap(NULL, _STARTING_MEMORY, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        *((size_t *)_my_mem) = _STARTING_MEMORY | (0b11); // is free, is the last chunk
    }
}

static inline size_t align(size_t size) {
    if ((size & 0b11) != 0) {
        return (size | (0b11)) + 1;
    }
    return size;
}

void *my_malloc(size_t size) {
    initialize_my_mem();
    size = align(size);
    // iterate over chunks to search for a free one that is big enough
    void *p = _my_mem;
    size_t chunk_size;
    do {
        chunk_size = *((size_t *)p);
        size_t real_chunk_size = chunk_size & (~0b11);

        size_t is_free = chunk_size & 0b01;
        if (is_free) {
            // merge with the following free chunks
            void *next_p = p + real_chunk_size;
            while (*(size_t *)next_p & 0b01) {
                // merge

                size_t real_next_size = *(size_t *)next_p & (~0b11);
                // increase the header and the saved size of the chunk
                *((size_t *)p) += real_next_size;
                real_chunk_size += real_next_size;

                if (*(size_t *)next_p & 0b10) {
                    // this was the last chunk
                    *((size_t *)p) |= 0b10;
                    chunk_size |= 0b10;
                    break;
                } else {
                    next_p += real_next_size;
                }
            }

            if (real_chunk_size >= size) {
                // found a matching chunk!
                if ((size - real_chunk_size) <= (sizeof(size_t) + 4)) {
                    // just take the same chunk
                    *((size_t *)p) &= (~0b1); // not free anymore
                    return p + sizeof(size_t);
                } else {
                    // split the chunk to two
                    size_t remaining_free_chunk_size = real_chunk_size - size - 2 * sizeof(size_t);
                    void *next_p = p + size + sizeof(size_t);
                    *((size_t *)next_p) = remaining_free_chunk_size + (chunk_size & 0b11); // same lastness, same freeness
                    *((size_t *)p) = size + sizeof(size_t); // not free, not last
                    return p + sizeof(size_t);
                }
            }
        }
        p += real_chunk_size;
    } while (!(chunk_size & 0b10)); // while not the last chunk
    // there is no space - TODO: expand memory in this case
    return NULL;
}

void my_free(void *ptr) {
    void *prev_ptr = ptr - sizeof(size_t);
    size_t value = *((size_t *)prev_ptr);
    *((size_t *)prev_ptr) = value | 0b01; // mark as free
}
// void *my_calloc(size_t nmemb, size_t size);
// void *my_realloc(void *ptr, size_t size);
