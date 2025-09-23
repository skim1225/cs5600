/*
 * polybius.h / Header for Polybius Cipher
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Sep 19, 2025
 *
 * Header file for encrypting and decrypting strings using
 * the Polybius square encryption algorithm.
 */

#ifndef POLYBIUS_H
#define POLYBIUS_H

#include <stddef.h>

// Struct for 5x5 Polybius square
typedef struct {
    char square[5][5];
} polybius_square_t;

/**
 * Encrypts a single character using the Polybius square.
 * - Handles 'I'/'J' as the same character.
 * - Only works on uppercase letters A–Z.
 *
 * @param c     Character to encrypt.
 * @param table Pointer to a Polybius square.
 * @return      Dynamically allocated string (2 chars + null terminator).
 */
char* get_cipher(char c, polybius_square_t *table);

/**
 * Encodes a plaintext string into Polybius cipher text.
 * - Upper/lowercase letters are supported.
 * - Non-alphabetic characters are preserved.
 *
 * @param plaintext Input text to encode.
 * @param table     Pointer to a Polybius square.
 * @return          Dynamically allocated ciphertext string.
 */
char* pbEncode(const char *plaintext, const polybius_square_t *table);

/**
 * Decodes a Polybius ciphertext string into plaintext (all caps).
 * - Interprets pairs of digits as coordinates in the square.
 * - Non-digit characters are preserved.
 *
 * @param ciphertext Input ciphertext to decode.
 * @param table      Pointer to a Polybius square.
 * @return           Dynamically allocated plaintext string.
 */
char* pbDecode(const char *ciphertext, const polybius_square_t *table);

#endif // POLYBIUS_H
