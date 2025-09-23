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

/**
 * @file
 * @brief Load and print dynamically sized integer blocks from a text file.
 *
 * @details
 * This program demonstrates a minimal memory-management workflow using a
 * heap-allocated container type, `dynBlock`, to hold an array of integers of
 * known length. It:
 *   - Opens a text file named `"blocks.data"` for reading.
 *   - Parses each non-empty line into a sequence of whitespace-separated
 *     integers.
 *   - Allocates a `dynBlock` sized to the number of integers on that line.
 *   - Copies the integers into the block and prints them for verification.
 *   - Frees all allocated memory before processing the next line.
 *
 * Empty lines are skipped. Any allocation failure is reported and handled
 * gracefully per-line so that the program can continue to process remaining
 * lines when possible.
 *
 * ### Expected input format
 * Each line contains zero or more integers separated by spaces and/or tabs.
 * Example:
 * @code
 * 1 2 3
 * 10    20 30  40
 *
 * 5
 * @endcode
 *
 * @note The program reads from a file named `"blocks.data"` in the working
 *       directory. Adjust the filename in `fopen()` if needed.
 *
 * @author Sooji Kim
 * @date   2025-09-16
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * @struct dynBlock
 * @brief Heap-allocated, fixed-length container for integers.
 *
 * @var dynBlock::len
 * Number of integers stored in this block.
 *
 * @var dynBlock::data
 * Pointer to the heap-allocated integer array (length is `len`).
 */
typedef struct {
    size_t len;  /**< Number of integers stored in this block. */
    int *data;   /**< Pointer to the heap-allocated integer array. */
} dynBlock;

/**
 * @brief Allocate a new @ref dynBlock with space for @p n integers.
 *
 * @details
 * Allocates the `dynBlock` object itself and, when @p n > 0, allocates a
 * contiguous integer array of length @p n. The `len` field is set to @p n.
 * If any allocation fails, previously allocated memory is freed and `NULL`
 * is returned.
 *
 * @param n Number of integers to allocate space for (may be zero).
 * @return Pointer to a newly allocated @ref dynBlock on success; `NULL` on failure.
 *
 * @note When @p n is zero, `data` is set to `NULL` and the block is still valid.
 * @warning The returned block must be released with freeDynBlock() to avoid leaks.
 */
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

/**
 * @brief Copy integers from @p src into an already-allocated @ref dynBlock.
 *
 * @details
 * Performs a shallow validation of inputs and verifies that the block length
 * matches @p n. On success, copies exactly @p n integers into `b->data`.
 *
 * @param b   Target block; must be non-NULL and have `len == n`.
 * @param src Source array of integers; must be non-NULL and at least @p n long.
 * @param n   Number of integers to copy (must equal `b->len`).
 *
 * @retval 0 Success; all @p n integers copied.
 * @retval 1 Failure; invalid arguments or length mismatch; no copy performed.
 *
 * @warning This function assumes `b->data` points to valid storage for `n` integers.
 * @note Use allocDynBlock() to create a block of the correct size beforehand.
 */
int storeMem2Blk(dynBlock *b, const int *src, size_t n) {
    if (!b || !src || b->len != n) {
        return 1;
    } else {
        memcpy(b->data, src, n * sizeof *src);
        return 0;
    }
}

/**
 * @brief Release all heap memory owned by a @ref dynBlock.
 *
 * @param b Pointer to the block to free; no effect if `NULL`.
 *
 * @post After return, @p b (and its `data`) are invalid and must not be used.
 */
void freeDynBlock(dynBlock *b) {
    if (!b) return;
    free(b->data);
    free(b);
}

/**
 * @brief Entry point: read lines from `"blocks.data"`, store into @ref dynBlock,
 * and print each line's integers.
 *
 * @details
 * For each non-empty line:
 *  - Count integers on the line.
 *  - Allocate a temporary array to parse tokens with `strtok`.
 *  - Allocate a @ref dynBlock of matching size.
 *  - Copy integers into the block with storeMem2Blk().
 *  - Print the contents as a verification step.
 *  - Free all temporary and block memory.
 *
 * @return `0` on success; non-zero on fatal errors (e.g., file open failure).
 *
 * @note Lines containing only whitespace are skipped.
 * @warning This routine uses `atoi` for simplicity; malformed tokens convert to 0.
 */
int main() {

    FILE *fp = fopen("blocks.data", "r");
    if (fp == NULL) {
        printf("Error opening file blocks.data\n");
        return 1;
    }

    char buf[4096];
    int num_lines = 0;

    while (fgets(buf, sizeof(buf), fp)) {
        num_lines++;

        // count ints in line
        int count = 0;
        char *copy = malloc(strlen(buf) + 1);
        if (!copy) {
            printf("Error allocating memory\n");
            fclose(fp);
            return 1;
        }
        strcpy(copy, buf);

        char *token = strtok(copy, " \t\r\n");
        while (token) {
            count++;
            token = strtok(NULL, " \t\r\n");
        }
        free(copy);

        // handle empty lines
        if (count == 0) {
            continue;
        }

        // temp int[]
        int *arr = malloc(count * sizeof *arr);
        if (!arr) {
            printf("Error allocating memory.\n");
            fclose(fp);
            return 1;
        }

        // fill arr from tokens
        int i = 0;
        token = strtok(buf, " \t\r\n");
        while (token && i < count) {
            arr[i++] = atoi(token);
            token = strtok(NULL, " \t\r\n");
        }

        // allocate dynBlock and copy ints into it
        dynBlock *blk = allocDynBlock(count);
        if (!blk) {
            printf("Error allocating memory.\n");
            free(arr);
            continue;
        }

        if (storeMem2Blk(blk, arr, count) != 0) {
            printf("Line %d: storeMemBlk failed.\n", num_lines);
            free(arr);
            freeDynBlock(blk);
            continue;
        }

        // print all ints from dynblock
        printf("Line %d (%d ints):", num_lines, count);
        for (int j = 0; j < count; j++) {
            printf(" %d", blk->data[j]);
        }
        printf("\n");

        free(arr);
        freeDynBlock(blk);
    }

    fclose(fp);
    return 0;
}
