/*
 * genWords-kim.c / Random Word Generation (hardcoded 10,000 words)
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Oct 3, 2025
 *
 * Program based on the random number generator assignment which generates
 * 10,000 "words" or strings and writes them to a file.
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/**
 * @brief Internal state for the pseudo-random number generator.
 */
static unsigned long curr_rand;

/**
 * @name Linear Congruential Generator (LCG) constants
 * These constants match the ANSI C implementation.
 * @{
 */
#define A 1103515245UL   /**< Multiplier constant for the LCG. */
#define C 12345UL        /**< Increment constant for the LCG. */
#define M 2147483648UL   /**< Modulus constant for the LCG (2^31). */
/** @} */


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
 * @brief Generate a random lowercase word and store it in a buffer.
 *
 * This function generates a word of random length between @p minLen and
 * @p maxLen, inclusive. Each character is a randomly selected lowercase
 * letter ('a'–'z'). The result is written into @p buf and null-terminated.
 *
 * @param buf Character buffer where the generated word will be stored.
 *            The buffer must be large enough to hold the maximum word
 *            length plus a null terminator.
 * @param minLen Minimum length of the word (inclusive).
 * @param maxLen Maximum length of the word (inclusive).
 */
static void genWord(char *buf, int minLen, int maxLen) {
    int len = genRand(minLen, maxLen);
    for (int i = 0; i < len; i++) {
        buf[i] = 'a' + genRand(0, 25);
    }
    buf[len] = '\0';
}


/**
 * @brief Entry point for the random word generator program.
 *
 * Seeds the pseudo-random generator with the current system time, then
 * generates 10,000 random words (each between 3 and 10 characters long).
 * The words are written to the file "words.txt", one per line.
 *
 * @return 0 on success, or 1 if the output file could not be opened.
 */
int main(void) {
    curr_rand = (unsigned long) time(NULL);

    FILE *fp = fopen("words.txt", "w");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    char buf[16];
    for (int i = 0; i < 10000; i++) {
        genWord(buf, 3, 10);
        fprintf(fp, "%s\n", buf);
    }

    fclose(fp);
    return 0;
}
