/*
* arger_utils.c / Helper library for arger-kim
 *
 * Implements character checks and error handling used by arger-kim.
 */

#include <stdio.h>
#include <stdlib.h>
#include "arger-utils.h"

/* Distance between ASCII upper and lower letters. */
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
