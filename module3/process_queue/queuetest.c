/*
 * queuetest.c / Process Queue — moderate test
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 23, 2025
 */

/**
 * @file queuetest.c
 * @brief Moderate test driver for the process queue library.
 *
 * This program demonstrates the usage of the custom generic queue implementation
 * defined in @c queue.c / @c queue.h. It creates several @c process_t objects,
 * enqueues them, prints the queue, removes elements using FIFO and priority-based
 * operations, and verifies correctness by printing results to stdout.
 *
 * @details
 * The test covers:
 *   - Queue initialization
 *   - Enqueuing processes with varying priorities
 *   - Removing elements using FIFO order via popQ()
 *   - Removing the highest-priority element via rmProcess()
 *   - Checking @c qsize() correctness throughout operations
 *   - Printing final queue state and cleanup
 *
 * @note
 * All dynamically allocated @c process_t objects are freed before program exit.
 *
 * @author
 *   Sooji Kim
 *
 * @date
 *   Fall 2025 (Sep 23, 2025)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

/**
 * @brief Allocate and initialize a new process object.
 *
 * Creates a @c process_t with the given PID, runtime, and priority.
 * The process is named "process_<pid>".
 *
 * @param[in] pid      Process identifier.
 * @param[in] runtime  Simulated runtime for the process.
 * @param[in] priority Scheduling priority (lower value = higher priority).
 *
 * @return Pointer to a heap-allocated @c process_t on success, or @c NULL if
 *         allocation fails.
 *
 * @post Caller is responsible for freeing the returned process using
 *       free_process().
 */
static process_t *make_process(int pid, long runtime, int priority) {
  process_t *p = (process_t *)malloc(sizeof *p);
  if (p == NULL) {
    printf("Failed to allocate memory for process\n");
    return NULL;
  }
  char buf[32];
  sprintf(buf, "process_%d", pid);
  p->pid = pid;
  p->name = strdup(buf);
  p->runtime = runtime;
  p->priority = priority;
  return p;
}

/**
 * @brief Free a process allocated with make_process().
 *
 * Releases the memory used by a @c process_t, including its dynamically
 * allocated name string.
 *
 * @param[in,out] p Pointer to the process to free. Safe to pass @c NULL.
 */
static void free_process(process_t *p) {
  if (p != NULL) {
    free(p->name);
    free(p);
  }
}

/**
 * @brief Print the contents of a queue to stdout with a label.
 *
 * Iterates through the queue and prints each @c process_t node’s PID, name,
 * and priority. If the queue is empty, prints "(empty)".
 *
 * @param[in] q     Pointer to the queue to print.
 * @param[in] label Descriptive string for expected contents.
 */
static void print_queue(queue_t *q, const char *label) {
  printf("EXPECT: %s\n", label);
  printf("Queue size: %d\n", qsize(q));

  node_t *curr = q->head;
  if (curr == NULL) {
    printf("(empty)\n\n");
    return;
  }

  while (curr != NULL) {
    process_t *process = (process_t *)curr->data;
    if (process != NULL) {
      printf("pid=%d name=%s priority=%d\n",
             process->pid, process->name, process->priority);
    }
    curr = curr->next;
  }
  printf("\n");
}

/**
 * @brief Entry point for queue tests.
 *
 * Runs a sequence of enqueue, dequeue, and priority-removal operations on
 * a @c queue_t and prints results to stdout for validation.
 *
 * @return Exit status code (0 = success).
 */
int main(void) {

  printf("Queue tests:\n");

  // init q
  queue_t q = (queue_t){0};
  printf("Test: Initial qsize (expect 0): %d\n", qsize(&q));
  print_queue(&q, "empty queue");

  // push 3 processes
  printf("Action: Enqueue three processes: process_1(priority=5), process_2(priority=3), process_3(priority=7)\n");
  add2q(&q, make_process(1, 10L, 5));
  add2q(&q, make_process(2, 20L, 3));
  add2q(&q, make_process(3, 30L, 7));
  print_queue(&q, "process_1(priority=5) -> process_2(priority=3) -> process_3(priority=7)");

  // test qsize - should be 3
  printf("Test: qsize after 3 enqueues (expect 3): %d\n", qsize(&q));

  // test popQ - should remove process 1
  printf("Test: popQ (expect process_1)\n");
  process_t *popped = (process_t *)popQ(&q);
  if (popped != NULL) {
    printf("Popped: pid=%d name=%s priority=%d\n", popped->pid, popped->name, popped->priority);
    free_process(popped);
  }
  print_queue(&q, "process_2(priority=3) -> process_3(priority=7)");

  // push 2 processes
  printf("Action: Enqueue process_4(priority=1) and process_5(priority=6)\n");
  add2q(&q, make_process(4, 15L, 1));
  add2q(&q, make_process(5, 50L, 6));
  print_queue(&q, "process_2(priority=3) -> process_3(priority=7) -> process_4(priority=1) -> process_5(priority=6)");

  // test rmProcess - should remove process 4
  printf("Test: rmProcess (expect process_4 with priority=1)\n");
  process_t *removed = rmProcess(&q);
  if (removed != NULL) {
    printf("Removed high-priority: pid=%d name=%s priority=%d\n",
           removed->pid, removed->name, removed->priority);
    free_process(removed);
  }
  print_queue(&q, "process_2(priority=3) -> process_3(priority=7) -> process_5(priority=6)");

  // Pop q until empty
  printf("Action: popQ until empty\n");
  while (qsize(&q) > 0) {
    process_t *d = (process_t *)popQ(&q);
    if (d != NULL) {
      printf("Popped: pid=%d name=%s priority=%d\n", d->pid, d->name, d->priority);
      free_process(d);
    }
  }
  printf("\n");
  print_queue(&q, "empty queue");

  printf("Test: Final qsize (expect 0): %d\n", qsize(&q));
  return 0;
}
