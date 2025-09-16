/*
* alloc-mem-kim.c / Memory Management
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 16, 2025
 *
 * Program which allocates various amounts of memory to different parts of
 * the RAM's memory addresses.
 */

#include <stdio.h>
#include <stdlib.h>

#define MB(x) ((size_t)(x) * 1024 * 1024)

// num ints reqd for each amount of memory
static const size_t STATIC_INTS = MB(5) / sizeof(int);
static const size_t STACK_INTS  = MB(1) / sizeof(int);
static const size_t HEAP_INTS   = MB(10) / sizeof(int);

// Allocate 5MB in static segment.
// Reserved at program load time and freed when process exits.
static int static_mem[STATIC_INTS];

int main(void) {
    // Allocate 1MB in stack when main() is called.
    // Memory returned to OS when main() returns.
    int stack_mem[STACK_INTS];

    // Allocate 10MB in the heap at runtime.
    int *heap_mem = malloc(HEAP_INTS * sizeof(int));
    if (heap_mem == NULL) {
        perror("malloc failed");
        exit(-1);
    }

    // Free the 10MB heap memory.
    free(heap_mem);

    return 0;
}

// When the program exits and the associated process is deleted, all memory
// (static, stack, heap) is reclaimed by the operating system.
