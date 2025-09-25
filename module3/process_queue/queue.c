/**
 * @file queue.c
 * @brief Generic doubly linked-list queue implementation with priority removal.
 *
 * This module implements a simple, generic queue backed by a doubly linked list.
 * Elements are stored as opaque pointers (`void *`) inside `node_t` nodes.
 * In addition to FIFO operations, it supports removing the first process with the
 * highest priority (numerically lowest `process_t::priority`).
 *
 * @author
 *   Sooji Kim
 *
 * @date
 *   Fall 2025 (Sep 23, 2025)
 *
 * @note
 *   Type definitions for @c queue_t, @c node_t, and @c process_t are expected
 *   to be provided by @c queue.h. This file does not allocate or free user
 *   payloads (the @c void* @c data inside nodes); ownership of element memory
 *   remains with the caller.
 *
 * @copyright
 *   For course use in CS5600, Northeastern University.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

/**
 * @brief Append one element to the tail of the queue.
 *
 * Allocates a new @c node_t, initializes it to wrap @p element, and links it
 * at the tail of @p queue. If the queue is empty, the new node becomes both
 * the head and the tail.
 *
 * @param[in,out] queue   Pointer to an initialized @c queue_t.
 * @param[in]     element Opaque pointer to the element to store. The function
 *                        does not copy or take ownership of the pointed-to data.
 *
 * @retval 0  Success; element appended and @c queue->size incremented.
 * @retval 1  Failure; @p queue was @c NULL or memory allocation failed. The
 *            queue remains unchanged on allocation failure.
 *
 * @pre  @p queue is either a valid, initialized queue or @c NULL.
 * @post On success, @p queue->tail points to the new node and @p queue->size
 *       is incremented by 1.
 *
 * @complexity O(1)
 */
int add2q(queue_t *queue, void *element) {
    if (queue == NULL) {
        printf("Queue is NULL\n");
        return 1;
    }

    // create new node
    node_t *new_node = malloc(sizeof(node_t));
    if (new_node == NULL) {
        printf("Failed to allocate memory for new_node\n");
        return 1;
    }

    // init newnode fields
    new_node->data = element;
    new_node->next = NULL;
    new_node->prev = queue->tail;

    // add newnode to q if q is not empty
    if (queue->tail != NULL) {
        queue->tail->next = new_node;
    } else {
        // empty queue: this is also the head
        queue->head = new_node;
    }

    // make new node the new tail of q
    queue->tail = new_node;
    queue->size += 1;
    return 0;
}

/**
 * @brief Remove and return the element at the head of the queue (FIFO pop).
 *
 * Unlinks the head node, frees the node structure, and returns the stored
 * @c void* payload without freeing it.
 *
 * @param[in,out] queue Pointer to an initialized @c queue_t.
 *
 * @return The payload pointer formerly at the head of the queue, or @c NULL if
 *         @p queue is @c NULL or empty.
 *
 * @post If the queue contained at least one element, @p queue->size is
 *       decremented by 1 and head/tail are updated accordingly.
 *
 * @warning The caller owns the returned payload and is responsible for any
 *          necessary deallocation of the underlying object.
 *
 * @complexity O(1)
 */
void* popQ(queue_t *queue) {

    // check for empty q
    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }

    node_t *old_head = queue->head;
    void *data = old_head->data;

    queue->head = old_head->next;

    // update q head, check if q is now empty
    if (queue->head != NULL) {
        queue->head->prev = NULL;
    } else {
        queue->tail = NULL;
    }

    free(old_head);
    queue->size -= 1;
    return data;
}

/**
 * @brief Remove and return the first process with the highest priority.
 *
 * Scans the queue for the node whose @c process_t::priority is the lowest
 * integer value. In case of ties, the earliest (closest to the head) such node
 * is selected. The node is unlinked and freed; the stored @c process_t* is
 * returned to the caller.
 *
 * @param[in,out] queue Pointer to an initialized @c queue_t that stores
 *                      @c process_t* payloads.
 *
 * @return Pointer to the removed @c process_t on success, or @c NULL if
 *         @p queue is @c NULL, empty, or contains no valid @c process_t.
 *
 * @post If a node was removed, @p queue->size is decremented by 1 and
 *       head/tail/links are adjusted.
 *
 * @note This function assumes that each node's @c data points to a valid
 *       @c process_t with an integer field @c priority where lower is higher
 *       priority.
 *
 * @complexity O(n) due to linear scan for priority selection.
 */
process_t* rmProcess(queue_t *queue) {

    // check for empty q
    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }

    int highest_priority = INT_MAX;
    node_t *highest_node = NULL;

    // iterate over q and find highest priority (lowest #)
    node_t *curr = queue->head;
    while (curr != NULL) {
        process_t *process = (process_t *)curr->data;
        if (process != NULL && process->priority < highest_priority) {
            highest_priority = process->priority;
            highest_node = curr;
            /* tie automatically prefers the first seen (earliest) */
        }
        curr = curr->next;
    }

    if (highest_node == NULL) {
        return NULL;
    }

    // unlink node from q
    if (highest_node->prev) {
        highest_node->prev->next = highest_node->next;
    } else {
        queue->head = highest_node->next;
    }
    if (highest_node->next) {
        highest_node->next->prev = highest_node->prev;
    } else {
        queue->tail = highest_node->prev;
    }

    process_t *process_to_return = (process_t *)highest_node->data;
    free(highest_node);
    if (queue->size > 0) {
        queue->size -= 1;
    }

    return process_to_return;
}

/**
 * @brief Get the number of elements currently in the queue.
 *
 * @param[in] queue Pointer to an initialized @c queue_t, or @c NULL.
 *
 * @return The number of elements if @p queue is non-NULL and non-empty;
 *         otherwise 0.
 *
 * @note If you need to distinguish between an empty queue and a @c NULL queue
 *       pointer, check @p queue first.
 *
 * @complexity O(1)
 */
int qsize(queue_t *queue) {
    if (queue == NULL || queue->head == NULL) {
        return 0;
    } else {
        return queue->size;
    }
}