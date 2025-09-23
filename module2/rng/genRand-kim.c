/*
* genRand-kim.c / Random Number Generation
*
* Sooji Kim / CS5600 / Northeastern University
* Fall 2025 / Sep 10, 2025
*
* Program for performing calculation and generating random numbers in C.
*/

#include <stdio.h>
#include <string.h>
#include <time.h>

static char APPEND[] = "-a";
static unsigned long random;

// LCM constants - same as ANSI C
#define A 1103515245UL
#define C 12345UL
#define M 2147483648UL  /* 2^31 */


/**
 * @brief Generate a pseudo-random integer within a specified range.
 *
 * This function uses a Linear Congruential Generator (LCG) to update the
 * internal random state and produce a new pseudo-random integer. The result
 * is then scaled to fit within the inclusive range [min, max].
 *
 * If @p min is greater than @p max, the two values are swapped so the range
 * is always valid. If @p min equals @p max, the function returns that value.
 *
 * @param min The lower bound of the desired range (inclusive).
 * @param max The upper bound of the desired range (inclusive).
 *
 * @return A pseudo-random integer in the range [min, max].
 */
int genRand(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    if (min == max) {
        return min;
    }
    random = ((A * random) + C) % M;
    int range = max - min + 1;
    double res = (double) random / (double) M;
    res *= range;
    res += min;
    return (int) res;
}

int main(int argc, char *argv[]) {

    random = (unsigned long) time(NULL);

    FILE *fp;
    int num = atoi(argv[1]);
    char filePath[] = argv[2];
    int overwrite = 1;

    if (argc > 3) {
        char option[] = argv[3];
        if ( strcmp(option, APPEND) == 0 ) {
            overwrite = 0;
        }
    }

    // open file in write or append mode depending on cmd ln arg
    if (overwrite) {
        fp = fopen(filePath, "w");
    } else {
        fp = fopen(filePath, "a");
    }

    // check if file was opened
    if (fp == NULL) {
        puts("Error opening file");
        exit(1);
    }

    // write to file the # of times specified
    for (int i = 0; i < num; i++) {
        fprintf(fp, "%d\n", genRand(0, 100));
    }

    // close file
    fclose(fp);

    return 0;
}