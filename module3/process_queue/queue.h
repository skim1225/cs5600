/*
 * queue.h / Implement Process Queue
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 23, 2025
 *
 * Header file for a custom generic queue structure.
 */

typedef struct {
    int identifier;
    char *name;
    long cum_runtime;
    int priority;
} process_t;

typedef struct {
    void * head;
} queue_t;