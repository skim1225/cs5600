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

static int OFFSET = 32;

int is_upper(char c) {
    if (c >= 'A' && c <= 'Z') {
        return 1;
    } else {
        return 0;
    }
}

int is_lower(char c) {
    if (c >= 'a' && c <= 'z') {
        return 1;
    } else {
        return 0;
    }
}

int main(int argc, char *argv[]) {
    char *choice = argv[1];
    char *word;
    int x;
    // to all uppercase
    if (strcmp(choice, "-u") == 0) {
        for (int i = 2; i < argc; i++) {
            word = argv[i];
            x = 0;
            while (word[x] != '\0') {
                if (is_lower(word[x])) {
                    word[x] -= OFFSET;
                }
                printf("%c", word[x]);
                x++;
            }
            if (i < argc - 1) {
                printf(" ");
            }
        }
        printf("\n");
    // to all lowercase
    } else if (strcmp(choice, "-l") == 0) {
        for (int i = 2; i < argc; i++) {
            word = argv[i];
            x = 0;
            while (word[x] != '\0') {
                if (is_upper(word[x])) {
                    word[x] += OFFSET;
                }
                printf("%c", word[x]);
                x++;
            }
            if (i < argc - 1) {
                printf(" ");
            }
        }
        printf("\n");
    // capitalize 1st letter of each word
    } else if (strcmp(choice, "-cap") == 0) {
        // check the first element
        if (is_lower(word[0])) {
            word[0] -= OFFSET;
        }
        x = 1;
        // loop over all other elements and check if previous char is " "
        while (word[x] != '\0') {
            // check if 1st letter of word
            if (word[x-1] == ' ') {
                if (is_lower(word[x])) {
                    word[x] -= OFFSET;
                }
            } else {
                if (is_upper(word[x])) {
                    word[x] += OFFSET;
                }
            }
            x++;
        }
        printf("%c", word[x]);
    } else {
        puts("Invalid choice. Valid flags are:");
        puts("-u: display text in all upper case.");
        puts("-l: display text in all lower case.");
        puts("-cap: display text with the first letter of each word capitalized.");
        exit(-1);
    }
    return 0;
}