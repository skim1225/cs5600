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

static char APPEND[] = "-a";

// TODO: IMPLEMENT A PSEUDO RNG ALGORITHM
int genRand(int min, int max) {
    return 0;
}

int main(int argc, char *argv[]) {
    FILE *fp;
    int num = atoi(argv[1]);
    char filePath[] = argv[2];
    int overwrite = 1;

    if ( argc > 3 ) {
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