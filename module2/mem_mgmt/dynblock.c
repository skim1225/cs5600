/**
* @file dynblock.c
 * @brief Implementation of dynamic block management functions.
 *
 * This file provides the function definitions for allocating, storing data in,
 * and freeing dynBlock structures that manage dynamic arrays of integers.
 */

#include "dynblock.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Allocate a dynBlock structure and its integer buffer.
 *
 * Allocates memory for a dynBlock and a contiguous buffer of @p n integers
 * from the heap. If @p n is zero, the data pointer will be NULL but the
 * structure is still valid.
 *
 * @param n Number of integers to allocate space for.
 * @return Pointer to the allocated dynBlock, or NULL if allocation fails.
 */
dynBlock* allocDynBlock(size_t n) {
    dynBlock *b = malloc(sizeof *b);
    if (b == NULL) {
        return NULL;
    }
    b->len = n;
    b->data = NULL;
    if (n != 0) {
        b->data = malloc(n * sizeof *b->data);
        if (b->data == NULL) {
            free(b);
            return NULL;
        }
    }
    return b;
}


/**
 * @brief Copy integers into a previously allocated dynBlock.
 *
 * Copies @p n integers from the source array into the target dynBlock. The
 * target dynBlock must already have space for exactly @p n integers.
 *
 * @param b   Pointer to the target dynBlock.
 * @param src Pointer to the source array of integers.
 * @param n   Number of integers to copy.
 * @return 0 on success, or -1 if @p b or @p src is NULL or if sizes mismatch.
 */
int storeMem2Blk(dynBlock *b, const int *src, size_t n) {
    if (!b || !src || b->len != n) return -1;
    if (n) memcpy(b->data, src, n * sizeof *src);
    return 0;
}

/**
 * @brief Free a dynBlock and its associated resources.
 *
 * Frees the integer buffer and the dynBlock structure itself. Safe to call
 * with a NULL pointer.
 *
 * @param b Pointer to the dynBlock to free.
 */
void freeDynBlock(dynBlock *b) {
    if (!b) return;
    free(b->data);
    free(b);
}
