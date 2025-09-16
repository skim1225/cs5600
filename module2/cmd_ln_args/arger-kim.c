/*
* arger-kim.c / Command Line Arguments
*
* Sooji Kim / CS5600 / Northeastern University
* Fall 2025 / Sep 10, 2025
*
* Program which accepts command line arguments to transform the case of some
* input text in various ways.
*/

#include <stdio.h>   /**< Standard I/O operations. */
#include <stdlib.h>
#include <string.h>

static const int OFFSET = 32;

int is_upper(char c) {
    return (c >= 'A' && c <= 'Z');
}

int is_lower(char c) {
    return (c >= 'a' && c <= 'z');
}

void error_message(void) {
    puts("Invalid choice. Valid flags are:");
    puts("-u: display text in all upper case.");
    puts("-l: display text in all lower case.");
    puts("-cap: display text with the first letter of each word capitalized.");
    exit(-1);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        error_message();
    }

    char *choice = argv[1];

    // display text in all uppercase
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
        return 0;
    // display text in all lowercase
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
        return 0;
    // capitalize first char of each word only
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
        return 0;
    } else {
        error_message();
    }
    return 0;
}
