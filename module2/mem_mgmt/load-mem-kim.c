/*
 * load-mem-kim.c / Memory Management
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 16, 2025
 *
 * Program for managing dynamic structures called dynBlocks. Includes reading
 * data from a file, saving it to dynBlocks, and printing the result for
 * verification purposes.
 */

#include <stdio.h>    /**< getline, FILE I/O */
#include <stdlib.h>   /**< malloc, realloc, free, exit */
#include <string.h>   /**< strerror, strlen, memcpy */
#include <errno.h>    /**< errno */

// stores an array of ints allocated dynamically to some specific and
// pre-defined size
typedef struct {
    size_t len;  /**< Number of integers stored in this block. */
    int *data; /**< Pointer to the heap-allocated integer array. */
} dynBlock;

// allocates a single instance of a dynBlock from the heap and returns a pointer
// to that allocated object
dynBlock* allocDynBlock(size_t n) {
    // init pointer to dynblock
    dynBlock *b = malloc(sizeof *b);
    if (b == NULL) {
        return NULL;
    }
    // init dynblock fields
    b->len = n;
    b->data = NULL;
    if (n != 0) {
        // init dynblock int arr
        b->data = malloc(n * sizeof *b->data);
        if (b->data == NULL) {
            free(b);
            return NULL;
        }
    }
    return b;
}

// stores the given array of integers to a previously allocated dynBlock
int storeMem2Blk(dynBlock *b, const int *src, size_t n) {
    if (!b || !src || b->len != n) {
        return 1;
    } else {
        memcpy(b->data, src, n * sizeof *src);
        return 0;
    }
}

int main() {
    // read ints from blocks.data
    // save each line into a separate dynBlock
    // demonstrate data was read
    return 0;
}