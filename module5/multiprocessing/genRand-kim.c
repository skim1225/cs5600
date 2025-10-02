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
#include <stdlib.h>

static char APPEND[] = "-a";
static unsigned long curr_rand;

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
    curr_rand = ((A * curr_rand) + C) % M;
    int range = max - min + 1;
    double res = (double) curr_rand / (double) M;
    res *= range;
    res += min;
    return (int) res;
}

/**
 * @brief Program entry point that generates pseudo-random integers and writes them to a file.
 *
 * @details
 * Parses command-line arguments, seeds the global PRNG state @c curr_rand with @c time(NULL),
 * opens the output file in overwrite or append mode, and writes @c num integers produced by
 * @c genRand(0, 100) (inclusive) to the file—one per line.
 *
 * **Usage**
 * @code
 *   ./genRand <count> <filename> [-a]
 *   // <count>   : number of integers to generate (non-negative integer)
 *   // <filename>: output file path
 *   // -a        : optional; append to the file instead of overwriting
 * @endcode
 *
 * @param argc Number of command-line arguments.
 * @param argv Argument vector:
 *             - @c argv[1] : count (string parsed to int)
 *             - @c argv[2] : output filename
 *             - @c argv[3] : optional flag @c "-a" to append
 *
 * @return
 * @retval 0 Success.
 * @retval 1 On insufficient arguments, file open failure, or file close error.
 *
 * @note This function initializes the global PRNG state variable @c curr_rand using @c time(NULL).
 * @warning This function assumes @c genRand is implemented and that @c APPEND equals the string "-a".
 */
int main(int argc, char *argv[]) {
    // validate args
    if (argc < 3) {
        printf("Insufficient arguments. Enter the number of random numbers "
               "you wish to generate and the name of the file you wish to "
               "write to. Include -a if you wish to append to the file "
               "instead of overwriting.\n");
        return 1;
    }

    // init vars
    curr_rand = (unsigned long) time(NULL);
    FILE *fp;
    int num = atoi(argv[1]);
    const char *filePath = argv[2];
    int overwrite = 1;
    if (argc > 3) {
        const char *option = argv[3];
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
        return(1);
    }

    // write to file the # of times specified
    for (int i = 0; i < num; i++) {
        fprintf(fp, "%d\n", genRand(0, 100));
    }

    // close file
    if (fclose(fp) != 0) {
        printf("Error closing file");
        return 1;
    }

    return 0;
}