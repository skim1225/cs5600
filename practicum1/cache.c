/*
 * cache.h / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * File containing the lookup, adding, and replacement logic for the cache.
 */

#include <stdio.h>
#include "cache.h"

void cache_init(cache_t *cache) {
    cache->use_counter = 0;
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache->entries[i].occupied = false;
        cache->entries[i].last_used = 0;
    }
}

message_t *cache_lookup(cache_t *cache, int id) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_entry_t *entry = &cache->entries[i];
        if (entry->occupied && entry->msg.content.id == id) {
            cache->use_counter++;
            entry->last_used = cache->use_counter;
            return &entry->msg;
        }
    }
    return NULL;
}

void replace_rand(cache_t *cache, const message_t *msg);
void replace_mru(cache_t *cache, const message_t *msg);

// insert msg into cache
void cache_insert(cache_t *cache, const message_t *msg, cache_policy_t policy) {

    // input validation
    if (!cache || !msg || !policy) {
        fprintf(stderr, "cache_insert: cache, msg, or policy is NULL\n");
        return;
    }

    // find unoccupied cache index
    int index = -1;
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (!cache->entries[i].occupied) {
            index = i;
            break;
        }
    }

    // if open slot, add msg to cache
    if (index >= 0) {
        cache_entry_t *entry = &cache->entries[index];
        entry->occupied = true;
        entry->msg = *msg;
        cache->use_counter++;
        entry->last_used = cache->use_counter;
        return;
    }

    // if cache full, replace an existing msg
    if (policy == CACHE_POLICY_RANDOM) {
        replace_rand(cache, msg);
    } else {
        replace_mru(cache, msg);
    }
}

// replaces a random message from the given cache with the given message
void replace_rand(cache_t *cache, const message_t *msg) {

    // input validation
    if (!cache || !msg) {
        fprintf(stderr, "replace_rand: cache or msg is NULL\n");
        return;
    }

    // pick random index to replace
    int victim = rand() % CACHE_SIZE;

    // replace msg
    cache_entry_t *entry = &cache->entries[victim];
    entry->msg = *msg;
    entry->occupied = true;
    cache->use_counter++;
    entry->last_used = cache->use_counter;
}

// replaces the most recently used msg in given cache with given msg
void replace_mru(cache_t *cache, const message_t *msg) {

    // input validation
    if (!cache || !msg) {
        fprintf(stderr, "replace_mru: cache or msg is NULL\n");
        return;
    }

    // find the entry with the largest last_used value
    unsigned long long max_last_used = 0;
    int mru_index = -1;

    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_entry_t *entry = &cache->entries[i];

        // get index of MRU msg/cache entry
        if (entry->occupied && entry->last_used > max_last_used) {
            max_last_used = entry->last_used;
            mru_index = i;
        }
    }

    if (mru_index == -1) {
        fprintf(stderr, "replace_mru: no occupied entries to replace\n");
        return;
    }

    // replace msg
    cache_entry_t *victim = &cache->entries[mru_index];
    victim->msg = *msg;
    victim->occupied = true;
    cache->use_counter++;
    victim->last_used = cache->use_counter;
}
