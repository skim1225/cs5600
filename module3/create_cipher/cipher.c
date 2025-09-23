/*
 * cipher.c / Create Cipher
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 19, 2025
 *
 * Program that takes an encryption flag and string plaintext from the command
 * line, encrypts it using the polybius square algorithm, and outputs the
 * ciphertext to the terminal.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "polybius.h"

static const polybius_square_t table = {
    .square = {
        {'A', 'B', 'C', 'D', 'E'},
        {'F', 'G', 'H', 'I', 'K'},
        {'L', 'M', 'N', 'O', 'P'},
        {'Q', 'R', 'S', 'T', 'U'},
        {'V', 'W', 'X', 'Y', 'Z'}
    }
};

int main(int argc, char* argv[]) {

    if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        char* ciphertext = pbEncode(argv[2], &table);
        if (ciphertext == NULL) {
            printf("There was an error during memory allocation. The program will now exit.\n");
            return 1;
        } else {
            printf("Ciphertext: %s\n", ciphertext);
            free(ciphertext);
        }

    } else {
        printf("Invalid arguments: use the flag -e to encrypt and place the string to encrypt within double quotes\n");
        return 1;
    }
    return 0;
}
