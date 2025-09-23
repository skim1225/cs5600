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

/**
 * @struct dynBlock
 * @brief Dynamic block of integers allocated on the heap.
 *
 * A dynBlock stores a length field and a pointer to a dynamically allocated
 * array of integers. The array size is fixed at allocation time.
 */
typedef struct {
    size_t len;  /**< Number of integers stored in this block. */
    int *data; /**< Pointer to the heap-allocated integer array. */
} dynBlock;

// allocates a single instance of a dynBlock from the heap and returns a pointer
// to that allocated object
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

// stores the given array of integers to a previously allocated dynBlock
void storeMem2Blk(int *ints, dynBlock *b) {
    b->len = sizeof(ints);
    b->data = ints;
}

/**
 * @brief Parse a single line of space-separated integers into a new dynBlock.
 *
 * The function performs two passes:
 *  1) Count how many integers are present on the line using @c strtol .
 *  2) Allocate a dynBlock of exactly that length and fill it with values.
 *
 * @param line Null-terminated C string containing one line of input.
 *             Newline characters should be trimmed prior to calling (not required).
 * @return Pointer to a newly allocated dynBlock on success; NULL on failure
 *         (e.g., allocation failure). The caller owns the returned object and
 *         must free it with ::freeDynBlock.
 */
static dynBlock* parse_line_to_block(const char *line)
{
    /* First pass: count integers */
    size_t count = 0;
    const char *p = line;
    char *end = NULL;

    for (;;) {
        errno = 0;
        (void)strtol(p, &end, 10);
        if (p == end) break;   /* no more numbers */
        ++count;
        p = end;
    }

    dynBlock *b = allocDynBlock(count);
    if (!b) return NULL;

    /* Second pass: fill values */
    p = line; end = NULL;
    for (size_t i = 0; i < count; ++i) {
        long v = strtol(p, &end, 10);
        /* Casting long -> int is acceptable for assignment constraints here. */
        b->data[i] = (int)v;
        p = end;
    }
    return b;
}

/**
 * @brief Program entry point.
 *
 * Reads lines from a data file (default: "blocks.data" or path from argv),
 * converts each line into a dynBlock, stores the blocks in a growable array,
 * prints them, and then releases all resources.
 *
 * @param argc Argument count.
 * @param argv Argument vector. If @c argc > 1, @c argv[1] is used as the input path.
 * @return 0 on success; non-zero on failure (e.g., file open error or allocation error).
 */
int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "blocks.data";
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return 1;
    }

    /* Growable array of dynBlock* (one per input line). */
    size_t cap = 8, nblocks = 0;
    dynBlock **blocks = (dynBlock**)malloc(cap * sizeof *blocks);
    if (!blocks) {
        fclose(fp);
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    /* Read file line-by-line using POSIX getline(). */
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        /* Trim trailing newline(s) for clean parsing/printing. */
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            line[--linelen] = '\0';
        }

        dynBlock *b = parse_line_to_block(line);
        if (!b) {
            fprintf(stderr, "Allocation failed while processing a line\n");
            break; /* fall through to cleanup */
        }

        if (nblocks == cap) {
            cap *= 2;
            dynBlock **tmp = (dynBlock**)realloc(blocks, cap * sizeof *tmp);
            if (!tmp) {
                fprintf(stderr, "Out of memory while growing block list\n");
                freeDynBlock(b);
                break; /* fall through to cleanup */
            }
            blocks = tmp;
        }
        blocks[nblocks++] = b;
    }

    free(line);
    fclose(fp);

    /* Demonstrate correctness by printing all blocks. */
    printf("Loaded %zu dynamic blocks from %s\n", nblocks, path);
    for (size_t i = 0; i < nblocks; ++i) {
        dynBlock *b = blocks[i];
        printf("Block %zu (len=%zu):", i, b->len);
        for (size_t j = 0; j < b->len; ++j) {
            printf(" %d", b->data[j]);
        }
        putchar('\n');
    }

    /* Release all heap allocations (blocks and array of pointers). */
    for (size_t i = 0; i < nblocks; ++i) {
        freeDynBlock(blocks[i]);
    }
    free(blocks);

    return 0;
}