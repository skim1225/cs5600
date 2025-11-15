/*
 * cache.h / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * Header files for the cache for Practicum I. Defines structs and functions of the cache.
 */

// PART 1: DESIGN A CACHE S.T. SOME NUMBER OF MESSAGES ARE STORED IN A PAGED STRUCTURE IN MEMORY

// TODO: Create appropriate lookup data structures to facilitate finding messages in the cache.
// In your code, thoroughly describe your strategy and design and why you chose it.
// Mention alternative designs and why you did not consider them.

#ifndef CACHE_H
#define CACHE_H

#include <stdbool.h>
#include "message.h"

#define CACHE_SIZE 16


typedef enum {
    CACHE_POLICY_RANDOM,
    CACHE_POLICY_MRU
} cache_policy_t;

typedef struct {
    bool occupied;
    message_t msg;
    unsigned long long last_used;
} cache_entry_t;

typedef struct {
    cache_entry_t entries[CACHE_SIZE];
    unsigned long long use_counter;
} cache_t;

extern cache_t g_cache;

void cache_init(cache_t *cache);
message_t *cache_lookup(cache_t *cache, int id);
void cache_insert(cache_t *cache, const message_t *msg, cache_policy_t policy);

#endif