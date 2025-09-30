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

int main(void)
{
    pthread_t th1, th2;
    pthread_create(&th1, NULL, worker, "X");
    pthread_create(&th2, NULL, worker, "Y");
    sleep(5);
    printf("Exiting from main program\n");
    return 0;
}



    /**
    1. (45 min / 40 pts) Read this tutorial on threads Links to an external site. and then implement the code from the tutorial and get it to run. You do not need to modify the code, just copy it as is. This will help you understand how threads are created and managed. Place the code in a source file named LastNameF.CS5600.F25.c where LastNameF is your last name and first name initial, e.g., SchedlbauerM.CS5600.F25.c.

    2. (15 min / 25 pts) Modify the code in the last example that "kills" or "cancels" threads and print out the thread ID in the printf() statements, e.g., change this line printf("====> Cancelling Thread Y!!\n"); so that it prints the thread's ID that being cancelled, then do the same for the other printf() statement.

    3. (15 min / 25 pts) Can threads "communicate", i.e., can they exchange or share data? One approach is to use a global array or structure to which all threads have access. This is the "shared memory" approach to thread communication. Expand the code example from (2) so that one thread adds some data to a global array of integers (but don't add the same value in all array elements) and the other thread reads that global data. It does not matter what the data is.

    4. (15 min / 10 pts) Observe the thread behavior from (3). Do you get consistency? Why is it not consistent? Would you expect it to be inconsistent? How would you avoid problems where threads step on each other? Provide a short explanation of the phenomena that you see. You do not have to resolve the problem, but only explain what the problem is and what it's resolution might require. Add a short explanation of 100-300 words to your code as a comment and be sure to label the comment so we know it is the answer to this question.

    */
