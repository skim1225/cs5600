/*
 * polybius.c / Create Cipher
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 19, 2025
 *
 * Library for encrypting and decrypting strings using the polybius square
 * encryption algorithm.
 */
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "polybius.h"

// takes only uppercase chars and returns the encrypted text
char* get_cipher(char c, const polybius_square_t *table) {
    char *encrypted = (char *)malloc(sizeof(char) * 3);
    if ( c == 'J' ) {
        c = 'I';
    }
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (c == table->square[i][j]) {
                encrypted[0] = '1' + i;
                encrypted[1] = '1' + j;
                encrypted[2] = '\0';
                return encrypted;
            }
        }
    }
}

// ignores special chars
// same int for I and J
char* pbEncode(const char *plaintext, const polybius_square_t *table) {
    int len = strlen(plaintext);
    char *ciphertext = (char *)malloc(sizeof(char) * (len * 2 + 1));
    int ciphertext_pos = 0;
    // TODO: ITERATE OVER STRING
    for (int i = 0; i < len; i++) {
        // TODO: IF UPPER, ITERATE OVER TABLE AND ADD ROW AND COL TO OUTPUT
        if (isupper(plaintext[i])) {
            ciphertext[ciphertext_pos] = get_cipher(plaintext[i], table);
            ciphertext_pos += 2;
        // TODO: IF LOWER, TOUPPER() THEN ITERATE OVER TABLE AND ADD ROW AND COL TO OUTPUT
        } else if (islower(plaintext[i])) {
            ciphertext[ciphertext_pos] = get_cipher(toupper(plaintext[i]), table);
            ciphertext_pos += 2;
        // TODO: IF SPECIAL CHAR, JUST ADD TO THE OUTPUT
        } else {
            ciphertext[i] = plaintext[i];
            ciphertext_pos += 1;
        }
    }
    return ciphertext;
}

// returns plaintext in all caps
char* pbDecode(const char *ciphertext, const polybius_square_t *table) {
    int len = strlen(ciphertext);
    char *decrypted = (char *)malloc(sizeof(char) * len);
    int decrypted_pos = 0;
    // TODO: iterate over ciphertext string
    for (int i = 0; i < len; i++) {
        // TODO: if it's a num, get 2 chars and convert to letter
        if (ciphertext[i] >= '0' && ciphertext[i] <= '9') {
            int row = ciphertext[i] - '0';
            int col = ciphertext[i + 1] - '0';
            decrypted[decrypted_pos] = table->square[row][col];
            i++;
            decrypted_pos += 1;
        // TODO: else, it's a special char and just add to the output
        } else {
            decrypted[decrypted_pos] = ciphertext[i];
            decrypted_pos += 1;
        }
    }
    return decrypted;
}