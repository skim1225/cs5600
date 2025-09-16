/*
 * arger-kim.c / Command Line Arguments
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 10, 2025
 *
 * Program which accepts command line arguments to transform the case of some
 * input text in various ways.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arger-utils.h"

int main(int argc, char *argv[]) {

    if (argc < 3) {
        error_message();
    }

    const int OFFSET = 32;
    char *choice = argv[1];

    /* -u: uppercase */
    if (strcmp(choice, "-u") == 0) {
        for (int i = 2; i < argc; i++) {
            char *word = argv[i];
            for (int j = 0; word[j] != '\0'; j++) {
                char c = word[j];
                if (is_lower(c)) {
                    c -= OFFSET;
                }
                putchar(c);
            }
            if (i < argc - 1) {
                putchar(' ');
            }
        }
        putchar('\n');

    /* -l: lowercase */
    } else if (strcmp(choice, "-l") == 0) {
        for (int i = 2; i < argc; i++) {
            char *word = argv[i];
            for (int j = 0; word[j] != '\0'; j++) {
                char c = word[j];
                if (is_upper(c)) {
                    c += OFFSET;
                }
                putchar(c);
            }
            if (i < argc - 1) {
                putchar(' ');
            }
        }
        putchar('\n');

    /* -cap: capitalize first char of each arg, lowercase the rest of that arg */
    } else if (strcmp(choice, "-cap") == 0) {
        for (int i = 2; i < argc; i++) {
            char *word = argv[i];
            if (word[0] != '\0') {
                char c0 = word[0];
                if (is_lower(c0)) {
                    c0 -= OFFSET;
                }
                putchar(c0);
                for (int j = 1; word[j] != '\0'; j++) {
                    char c = word[j];
                    if (is_upper(c)) c += OFFSET;
                    putchar(c);
                }
            }
            if (i < argc - 1) {
                putchar(' ');
            }
        }
        putchar('\n');
    } else {
        error_message();
    }
    return 0;
}
