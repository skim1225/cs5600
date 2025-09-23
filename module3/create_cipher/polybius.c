/*
 * polybius.c / Create Cipher
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 19, 2025
 *
 * Library for encrypting and decrypting strings using the polybius square
 * encryption algorithm.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "polybius.h"

/**
 * @brief Encode a single uppercase letter using the Polybius square.
 *
 * Converts an uppercase alphabetic character @p c into its Polybius
 * coordinates and writes the two digit characters ('1'..'5') into
 * @p encrypted. In this cipher, 'J' is treated as 'I'.
 *
 * @param c         Uppercase letter 'A'..'Z' to encode. 'J' is mapped to 'I'.
 * @param table     Pointer to a 5x5 Polybius square (must not be NULL).
 * @param encrypted Output buffer with capacity for at least 2 bytes
 *                  (must not be NULL). On success:
 *                  - encrypted[0] == '1' + row (0..4)
 *                  - encrypted[1] == '1' + col (0..4)
 *
 * @return int      0 on success; 1 on failure (e.g., @p table or @p encrypted
 *                  is NULL, or @p c is not found in @p table).
 *
 * @pre The caller guarantees @p c is an uppercase alphabetic character.
 * @pre @p encrypted points to at least 2 writable bytes.
 * @pre @p table represents the same alphabet variant used by the caller
 *      (e.g., I/J merged with 'J' omitted).
 *
 * @post On success, exactly two bytes are written to @p encrypted and no
 *       other memory is modified.
 */
int get_cipher(char c, const polybius_square_t *table, char encrypted[2]) {
    // validate input
    if (table == NULL || encrypted == NULL) {
        return 1;
    }
    // I and J both represented by I in this cipher
    if ( c == 'J' ) {
        c = 'I';
    }
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (c == table->square[i][j]) {
                encrypted[0] = '1' + i;
                encrypted[1] = '1' + j;
                return 0;
            }
        }
    }
    // if char not found in table (shouldn't ever be reached)
    return 1;
}

/**
 * @brief Encode a plaintext string using the Polybius square.
 *
 * For each alphabetic character in @p plaintext, the character is uppercased
 * and encoded into a two-digit coordinate ('1'..'5','1'..'5') using the
 * provided Polybius @p table via ::get_cipher. Non-alphabetic characters are
 * **not encoded**; they are copied through to the output unchanged.
 *
 * The returned ciphertext is a newly allocated, NUL-terminated string whose
 * length is at most (2 × strlen(@p plaintext)) + 1 bytes. The caller owns the
 * memory and must free it.
 *
 * @param plaintext  NUL-terminated input string to encode (must not be NULL).
 * @param table      Pointer to a 5×5 Polybius square used for encoding
 *                   (must not be NULL).
 *
 * @return char*     Pointer to a newly allocated ciphertext string on success;
 *                   NULL on invalid arguments, allocation failure, or if an
 *                   internal character-encoding step fails.
 *
 * @pre @p plaintext and @p table are non-NULL.
 * @post On success, the returned string is NUL-terminated and contains digit
 *       pairs for letters; non-letters appear unchanged.
 *
 * @note This function relies on ::get_cipher to map letters to digit pairs
 *       (with 'J' treated as 'I' by convention).
 */
char* pbEncode(const char *plaintext, const polybius_square_t *table) {
    // input validation
    if (plaintext == NULL || table == NULL) {
        printf("Invalid arguments.\n");
        return NULL;
    }
    int len = strlen(plaintext);
    char *ciphertext = (char *)malloc(sizeof(char) * (len * 2 + 1));
    if (ciphertext == NULL) {
        printf("There was an error during memory allocation.\n");
        return NULL;
    }
    int ciphertext_pos = 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char) plaintext[i];
        if (isalpha(c)) {
            char encrypted[2];
            int res = get_cipher(toupper(c), table, encrypted);
            if (res == 0) {
                ciphertext[ciphertext_pos] = encrypted[0];
                ciphertext[ciphertext_pos + 1] = encrypted[1];
                ciphertext_pos += 2;
            } else {
                printf("There was an error during encryption.\n");
                free(ciphertext);
                return NULL;
            }
        } else {
            ciphertext[ciphertext_pos] = c;
            ciphertext_pos += 1;
        }
    }
    ciphertext[ciphertext_pos] = '\0';
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