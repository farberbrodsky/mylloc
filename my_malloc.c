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

// Returns the current real chunk size of the free chunk
static size_t merge_free(void *free_ptr) {
    void *next_p = free_ptr + (*(size_t *)free_ptr & (~0b11));
    while (*(size_t *)next_p & 0b01) {
        size_t real_next_size = *(size_t *)next_p & (~0b11);
        // increase the header
        *((size_t *)free_ptr) += real_next_size;

        if (*(size_t *)next_p & 0b10) {
            // this was the last chunk
            *((size_t *)free_ptr) |= 0b10;
            break;
        } else {
            next_p += real_next_size;
        }
    }
    return *(size_t *)free_ptr & (~0b11);
}

void *my_malloc(size_t size) {
    initialize_my_mem();
    size = align(size);
    // iterate over chunks to search for a free one that is big enough
    void *p = _my_mem;
    size_t chunk_size;
    size_t real_chunk_size;
    do {
        chunk_size = *((size_t *)p);
        real_chunk_size = chunk_size & (~0b11);

        size_t is_free = chunk_size & 0b01;
        if (is_free) {
            // if this is a free chunk, merge it with the following ones
            if (*(size_t *)p & 0b01) {
                real_chunk_size = merge_free(p);
                chunk_size = *(size_t *)p;
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
    // there is no space - expand memory
    // p is now the last chunk
    p -= real_chunk_size;

    // p - _my_mem is the amount of memory we used
    size_t true_allocated = p - _my_mem;
    mremap(_my_mem, allocated, true_allocated + sizeof(size_t) + size, 0);
    allocated = true_allocated + sizeof(size_t) + size;

    // if the last chunk was free - expand it
    if (*(size_t *)p & 0b01) {
        *(size_t *)p += size;
        *(size_t *)p &= (~0b01); // not free anymore
        return p + sizeof(size_t);
    } else {
        // create a new chunk
        *(size_t *)p &= (~0b10); // not last anymore
        p += real_chunk_size;
        *(size_t *)p = size | 0b10; // is last, with this size
        return p + sizeof(size_t);
    }
}

void my_free(void *ptr) {
    void *prev_ptr = ptr - sizeof(size_t);
    size_t value = *((size_t *)prev_ptr);
    *((size_t *)prev_ptr) = value | 0b01; // mark as free
}

void *my_calloc(size_t nmemb, size_t size) {
    size_t real_size = align(nmemb * size);
    void *p = my_malloc(real_size);
    void *end = p + real_size;
    // fill with zeroes
    for (; p < end; ++p) *(unsigned char *)p = 0;
    return p;
}

void *my_realloc(void *ptr, size_t size) {
    size = align(size);
    void *p = ptr - sizeof(size_t);

    size_t real_chunk_size = *(size_t *)p & (~0b11);
    if (real_chunk_size > size) {
        // add free space after it
        if ((real_chunk_size - size) < sizeof(size_t)) {
            // adding free space just isn't worth it
            return ptr;
        }
        void *next_chunk = p + real_chunk_size;

        if (*(size_t *)p & 0b10) {
            // p was last
            *(size_t *)p &= (~0b10);
            *(size_t *)next_chunk = (real_chunk_size - size) | 0b11;
        } else {
            *(size_t *)next_chunk = (real_chunk_size - size) | 0b01;
        }
        return ptr;
    } else {
        // try to expand the chunk
        void *next_ptr = p + real_chunk_size;
        if (*(size_t *)next_ptr & 0b01) {
            size_t next_chunk_size = merge_free(next_ptr);
            if (real_chunk_size + next_chunk_size >= size) {
                // can expand in-place
                *(size_t *)p += next_chunk_size;
                return ptr;
            }
        }
        // expand by copying somewhere else
        void *result = my_malloc(size);
        memcpy(ptr, result, real_chunk_size);
        my_free(ptr);
        return result;
    }
}
