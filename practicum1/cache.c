/*
 * cache.c / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * File containing the lookup, adding, and replacement logic for the cache.
 */

#include <stdio.h>
#include <stdlib.h>
#include "cache.h"

/**
 * @brief Initialize the message cache.
 *
 * Sets the use counter to 0 and marks all cache entries as unoccupied
 * with their last_used timestamp reset.
 *
 * @param cache Pointer to the cache structure to initialize.
 */
void cache_init(cache_t *cache) {
    cache->use_counter = 0;
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache->entries[i].occupied = false;
        cache->entries[i].last_used = 0;
    }
}

/**
 * @brief Look up a message in the cache by message id.
 *
 * Scans the cache for an occupied entry with a matching message id.
 * If found, updates the cache use counter and the entry's last_used
 * value, then returns a pointer to the cached message.
 *
 * @param cache Pointer to the cache to search.
 * @param id    Message identifier to look up.
 *
 * @return Pointer to the cached message on success, or NULL if not found.
 */
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

/**
 * @brief Replace a random cache entry with a new message.
 *
 * Selects a random index in the cache and overwrites that entry
 * with the provided message, updating occupancy and usage metadata.
 *
 * @param cache Pointer to the cache in which to perform the replacement.
 * @param msg   Pointer to the message to insert into the cache.
 */
void replace_rand(cache_t *cache, const message_t *msg);

/**
 * @brief Replace the most recently used (MRU) cache entry with a new message.
 *
 * Finds the cache entry with the largest last_used value (i.e., most recently
 * used) and overwrites that entry with the provided message, updating
 * occupancy and usage metadata.
 *
 * @param cache Pointer to the cache in which to perform the replacement.
 * @param msg   Pointer to the message to insert into the cache.
 */
void replace_mru(cache_t *cache, const message_t *msg);

/**
 * @brief Insert a message into the cache using the specified replacement policy.
 *
 * Attempts to place the message into an unused cache slot. If no free slot is
 * available, uses the given replacement policy (random or MRU) to evict an
 * existing entry and store the new message in its place.
 *
 * Performs basic input validation and logs an error to stderr if the cache
 * or message pointers are NULL.
 *
 * @param cache  Pointer to the cache into which the message is inserted.
 * @param msg    Pointer to the message to be cached.
 * @param policy Cache replacement policy to use when the cache is full.
 */
void cache_insert(cache_t *cache, const message_t *msg, cache_policy_t policy) {

    // input validation
    if (!cache || !msg ) {
        fprintf(stderr, "cache_insert: cache or msg is NULL\n");
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

/**
 * @brief Replace a random message from the cache with the given message.
 *
 * Performs basic input validation and selects a random cache index as the
 * victim. The victim entry is overwritten with the provided message, marked
 * as occupied, and its last_used value is updated.
 *
 * @param cache Pointer to the cache where replacement is performed.
 * @param msg   Pointer to the message to insert into the selected entry.
 */
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

/**
 * @brief Replace the most recently used message in the cache with the given message.
 *
 * Identifies the cache entry with the highest last_used value among occupied
 * entries and overwrites that entry with the provided message. If no occupied
 * entries exist, logs an error to stderr and returns without modification.
 *
 * @param cache Pointer to the cache where replacement is performed.
 * @param msg   Pointer to the message to insert into the MRU entry.
 */
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