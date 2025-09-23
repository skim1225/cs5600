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

// helper func to free dynblock heap mem
void freeDynBlock(dynBlock *b) {
    if (!b) return;
    free(b->data);
    free(b);
}

int main() {
    FILE *fp = fopen("blocks.data", "r");
    if (fp == NULL) {
        perror("blocks.data");
        return 1;
    }

    enum { MAX_LINE = 4096 };
    char buf[MAX_LINE];
    int num_lines = 0;

    while (fgets(buf, sizeof buf, fp)) {
        num_lines++;

        // count tokens on this line
        int count = 0;
        char *copy = strdup(buf);
        if (!copy) {
            perror("strdup");
            fclose(fp);
            return 1;
        }
        char *tok = strtok(copy, " \t\r\n");
        while (tok) {
            count++;
            tok = strtok(NULL, " \t\r\n");
        }
        free(copy);
        if (count == 0) continue;

        // allocate dynBlock
        dynBlock *blk = allocDynBlock(count);
        if (!blk) {
            fprintf(stderr, "Line %d: allocDynBlock(%d) failed\n", num_lines, count);
            continue;
        }

        // fill block directly
        int i = 0;
        tok = strtok(buf, " \t\r\n");
        while (tok && i < count) {
            blk->data[i++] = atoi(tok);
            tok = strtok(NULL, " \t\r\n");
        }

        // print all integers in this block
        printf("Line %d (%d ints):", num_lines, count);
        for (int j = 0; j < count; j++) {
            printf(" %d", blk->data[j]);
        }
        printf("\n");

        freeDynBlock(blk);
    }

    fclose(fp);
    return 0;
}
