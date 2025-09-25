/*
 * queue.h / Implement Process Queue
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 23, 2025
 *
 * Header file for a custom generic queue structure.
 */

#ifndef QUEUE_H
#define QUEUE_H

/**
 * @brief Representation of a process.
 *
 * Each process has an integer identifier, a name string, cumulative run-time,
 * and a scheduling priority. Lower values of @c priority indicate higher
 * scheduling priority.
 */
typedef struct {
    int   pid;       /**< Unique identifier for the process. */
    char* name;      /**< Dynamically allocated process name string. */
    long  runtime;   /**< Cumulative run-time for this process. */
    int   priority;  /**< Priority (lower number = higher priority). */
} process_t;

/**
 * @brief A node in the doubly-linked queue.
 *
 * Each node stores a generic data pointer (@c void*), and has forward/backward
 * links to neighboring nodes.
 */
typedef struct node {
    void*        data; /**< Opaque pointer to the stored element. */
    struct node* prev; /**< Pointer to the previous node, or NULL if head. */
    struct node* next; /**< Pointer to the next node, or NULL if tail. */
} node_t;

/**
 * @brief Queue structure implemented as a doubly-linked list.
 *
 * The queue maintains pointers to its head and tail nodes for O(1) enqueue
 * and dequeue operations, and tracks its current size.
 */
typedef struct {
    node_t* head; /**< Pointer to the first node (head) in the queue. */
    node_t* tail; /**< Pointer to the last node (tail) in the queue. */
    int     size; /**< Number of elements currently in the queue. */
} queue_t;

/* ---------------- Queue API ---------------- */

/**
 * @brief Append one element to the tail of the queue.
 *
 * Allocates a new @c node_t, initializes it to wrap @p element, and links it
 * at the tail of @p queue. If the queue is empty, the new node becomes both
 * the head and the tail.
 *
 * @param[in,out] queue   Pointer to an initialized @c queue_t.
 * @param[in]     element Pointer to the element to store.
 *
 * @retval 0 Success; element appended and @c queue->size incremented.
 * @retval 1 Failure; @p queue was @c NULL or memory allocation failed.
 */
int add2q(queue_t* queue, void* element);

/**
 * @brief Remove and return the element at the head of the queue (FIFO).
 *
 * Unlinks the head node, frees the node structure, and returns the stored
 * payload pointer without freeing it.
 *
 * @param[in,out] queue Pointer to an initialized @c queue_t.
 *
 * @return Payload pointer formerly at the head of the queue, or @c NULL if
 *         the queue is @c NULL or empty.
 */
void* popQ(queue_t* queue);

/**
 * @brief Remove and return the first process with the highest priority.
 *
 * Scans the queue for the node whose @c process_t::priority is the lowest
 * integer value. In case of ties, the earliest such node is selected. The
 * node is unlinked and freed; the stored @c process_t* is returned.
 *
 * @param[in,out] queue Pointer to a queue storing @c process_t* payloads.
 *
 * @return Pointer to the removed @c process_t, or @c NULL if the queue is
 *         @c NULL, empty, or contains no valid process.
 */
process_t* rmProcess(queue_t* queue);

/**
 * @brief Get the number of elements currently in the queue.
 *
 * @param[in] queue Pointer to an initialized @c queue_t, or @c NULL.
 *
 * @return The number of elements in the queue; 0 if @p queue is @c NULL
 *         or empty.
 */
int qsize(queue_t* queue);

#endif /* QUEUE_H */