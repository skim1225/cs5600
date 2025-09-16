#ifndef ARGER_UTILS_H
#define ARGER_UTILS_H

#include <stdio.h>

/**
 * @file arger_utils.h
 * @brief Declarations for character-case helpers and error handling.
 */

/**
 * @brief Check if a character is uppercase.
 * @param c Character to test.
 * @return 1 if c is 'A'..'Z', else 0.
 */
int is_upper(char c);

/**
 * @brief Check if a character is lowercase.
 * @param c Character to test.
 * @return 1 if c is 'a'..'z', else 0.
 */
int is_lower(char c);

/**
 * @brief Print an error message and terminate with exit code -1.
 * @return This function does not return.
 */
void error_message(void);

#endif /* ARGER_UTILS_H */
