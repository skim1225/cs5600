/*
 * KimS.CS5600.F25.c / Create Threads
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 29, 2025
 *
 * Program for exploring threads in C.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/* 1. (45 min / 40 pts) Read this tutorial on threads Links to an external site.
 and then implement the code from the tutorial and get it to run. You do not
 need to modify the code, just copy it as is. This will help you understand how
 threads are created and managed. Place the code in a source file named
 LastNameF.CS5600.F25.c where LastNameF is your last name and first name initial,
 e.g., SchedlbauerM.CS5600.F25.c. */

void *worker(void *data)
{
    char *name = (char*)data;

    for (int i = 0; i < 120; i++)
    {
        usleep(50000);
        printf("Hi from thread name = %s\n", name);
    }

    printf("Thread %s done!\n", name);
    return NULL;
}

/* EX: THREAD CREATION IN C
int main(void)
{
    pthread_t th1, th2;
    pthread_create(&th1, NULL, worker, "X");
    pthread_create(&th2, NULL, worker, "Y");
    sleep(5);
    printf("Exiting from main program\n");
    return 0;
}
*/

/* EX: USE OF PTHREAD_JOIN()
int main(void)
{
    pthread_t th1, th2;
    pthread_create(&th1, NULL, worker, "X");
    pthread_create(&th2, NULL, worker, "Y");
    sleep(5);
    printf("exiting from main program\n");
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    return 0;
}
*/

/* EX: THREAD TERMINATION WITH PTHREAD_CANCEL()
int main(void)
{
    pthread_t th1, th2;
    pthread_create(&th1, NULL, worker, "X");
    pthread_create(&th2, NULL, worker, "Y");
    sleep(1);
    printf("====> Cancelling Thread Y!!\n");
    pthread_cancel(th2);
    usleep(100000);
    printf("====> Cancelling Thread X!\n");
    pthread_cancel(th1);
    printf("exiting from main program\n");
    return 0;
}
*/

/* 2. (15 min / 25 pts) Modify the code in the last example that "kills" or
 "cancels" threads and print out the thread ID in the printf() statements, e.g.,
 change this line printf("====> Cancelling Thread Y!!\n"); so that it prints
 the thread's ID that being cancelled, then do the same for the other printf()
 statement.

int main(void)
{
    pthread_t th1, th2;
    pthread_create(&th1, NULL, worker, "X");
    pthread_create(&th2, NULL, worker, "Y");
    sleep(1);
    printf("====> Cancelling Thread ID %lu\n", (unsigned long)th2);
    pthread_cancel(th2);
    usleep(100000);
    printf("====> Cancelling Thread ID %lu\n", (unsigned long)th1);
    pthread_cancel(th1);
    printf("exiting from main program\n");
    return 0;
}
*/

/* 3. (15 min / 25 pts) Can threads "communicate", i.e., can they exchange or
 share data? One approach is to use a global array or structure to which all
 threads have access. This is the "shared memory" approach to thread communication.
 Expand the code example from (2) so that one thread adds some data to a global
 array of integers (but don't add the same value in all array elements) and the
 other thread reads that global data. It does not matter what the data is.
*/

#define SIZE 20

int global_arr[SIZE];
// TODO: WRITE START ROUTINE FOR THREAD 1 TO ADD DATA TO GLOBAL INT ARR
void *worker1(void *data)
{

    for (int i = 0; i < SIZE; i++)
    {
        usleep(50000);
        global_arr[i] = i;
        printf("Writer writing to global_arr[%d] = %d\n", i, global_arr[i]);
    }

    printf("Thread %lu done!\n", (unsigned long)pthread_self());
    return NULL;
}

// TODO: WRITE START ROUTINE FOR THREAD 2 TO READ DATA FROM GLOBAL INT ARR
void *worker2(void *data)
{

    for (int i = 0; i < SIZE; i++)
    {
        usleep(50000);
        printf("Reader sees: global_arr[%d] = %d\n", i, global_arr[i]);
    }

    printf("Thread %lu done!\n", (unsigned long)pthread_self());
    return NULL;
}
// TODO: WRITE MAIN FUNC FOR CREATING THREADS
int main(void)
{
    pthread_t th1, th2;
    pthread_create(&th1, NULL, worker1, NULL);
    pthread_create(&th2, NULL, worker2, NULL);
    sleep(5);
    printf("Exiting from main program\n");
    return 0;
}

/* 4. (15 min / 10 pts) Observe the thread behavior from (3). Do you get
 consistency? Why is it not consistent? Would you expect it to be inconsistent?
 How would you avoid problems where threads step on each other? Provide a short
 explanation of the phenomena that you see. You do not have to resolve the problem,
 but only explain what the problem is and what it's resolution might require.
 Add a short explanation of 100-300 words to your code as a comment and be sure
 to label the comment so we know it is the answer to this question.

- not consistent
- due to differences in how cpu schedules
- avoid by using waiting or making one thread wait for result of another or join()
- problems - inconsistency, race conditions

*/

/* =========================
 * ANSWER TO Q4 (Consistency)
 * =========================
 * Observation: When threads share memory without synchronization, results are
 * often inconsistent: readers may print partially updated arrays, repeated
 * values, out-of-order diagnostics, or “missing” updates. This happens even on
 * a single CPU due to compiler optimizations and instruction reordering; on
 * multicore systems, caches can make writes by one thread invisible to another
 * for a while.
 *
 * Why it’s not consistent: Unsynchronized reads/writes create data races. The
 * compiler and CPU may reorder memory operations, and cores can read stale
 * cache lines. Without a “happens-before” edge, there is no guarantee that the
 * reader observes a complete, single round of data. Printing interleaves
 * nondeterministically, so output ordering differs across runs.
 *
 * Should we expect inconsistency? Yes. The C memory model says that racy
 * programs have undefined behavior; inconsistent results (or seemingly
 * “impossible” states) are expected.
 *
 * Avoiding “stepping on each other”: Establish proper synchronization. Protect
 * shared state (the array and version flag) with a mutex, and coordinate
 * transfer with a condition variable (or semaphores). These primitives create
 * a happens-before relationship that prevents torn reads and stale data. Other
 * valid approaches include atomics with acquire/release semantics, barriers,
 * or message passing (queues/channels). In this program, the mutex+condvar
 * “publish/subscribe” pattern ensures the reader copies a coherent snapshot
 * only after the writer has fully published it.
 */
