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
#include <string.h>

static int OFFSET = 32;

int isupper(char c) {
    if (c >= 'A' && c <= 'Z') {
        return 1;
    } else {
        return 0;
    }
}

int islower(char c) {
    if (c >= 'a' && c <= 'z') {
        return 1;
    } else {
        return 0;
    }

int main(int argc, char *argv[]) {
    char choice[] = argv[1];
    char str[] = argv[2];
    int x = 0;
    if (strcmp(choice, "-u") == 0) {
        while (str[x] != '\0') {
            if (islower(str[x])) {
                str[x] -= OFFSET;
            }
            x++;
        }
        printf("%s\n", str);
    } else if (strcmp(choice, "-l") == 0) {
        while (str[x] != '\0') {
            if (isupper(str[x])) {
                str[x] += OFFSET;
            }
            x++;
        }
        printf("%s\n", str);
    } else if (strcmp(choice, "-cap") == 0) {
        // check the first element
        if (islower(str[0])) {
            str[0] -= OFFSET;
        }
        x = 1;
        // loop over all other elements and check if previous char is " "
        while (str[x] != '\0') {
            // check if 1st letter of word
            if (strcmp(str[x-1]," ") == 0) {
                if (islower(str[x])) {
                    str[x] -= OFFSET;
                }
            } else {
                if (isupper(str[x])) {
                    str[x] += OFFSET;
                }
            }
            x++;
        }
        printf("%s\n", str);
    } else {
        puts("Invalid choice. Valid flags are:");
        puts("-u: display text in all upper case.");
        puts("-l: display text in all lower case.");
        puts("-cap: display text with the first letter of each word capitalized.");
        exit(-1);
    }
    return 0;
}