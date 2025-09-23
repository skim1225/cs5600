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

/**
 * @brief 5x5 Polybius square (typically with I/J merged).
 *
 * By convention, many Polybius squares omit 'J' and map it to 'I'.
 * This library relies on whatever mapping is present in the table.
 */
typedef struct {
    char square[5][5];
} polybius_square_t;

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
 */
int get_cipher(char c, const polybius_square_t *table, char encrypted[2]);

/**
 * @brief Encode a plaintext string using the Polybius square.
 *
 * Letters are uppercased and encoded to two-digit coordinates; non-letters
 * are copied through unchanged. Returns a newly allocated, NUL-terminated
 * string the caller must free.
 *
 * @param plaintext  NUL-terminated input string (must not be NULL).
 * @param table      Pointer to a 5×5 Polybius square (must not be NULL).
 * @return char*     Newly allocated ciphertext on success; NULL on error.
 */
char* pbEncode(const char *plaintext, const polybius_square_t *table);

/**
 * @brief Decode a Polybius-digit ciphertext into plaintext (uppercase letters).
 *
 * Treats any '1'..'5' digit pair as (row,col) into @p table; non-digits are
 * copied through unchanged. Returns a newly allocated, NUL-terminated string
 * the caller must free.
 *
 * @param ciphertext NUL-terminated input string (must not be NULL).
 * @param table      Pointer to a 5×5 Polybius square (must not be NULL).
 * @return char*     Newly allocated decoded string on success; NULL on error.
 */
char* pbDecode(const char *ciphertext, const polybius_square_t *table);

#endif /* POLYBIUS_H */