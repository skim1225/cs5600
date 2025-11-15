/*
 * main.c / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * This program simulates message caching, storage, and retrieval to explore
 * memory hierarchies and page replacement algorithms.
 */

#include <stdio.h>
#include <stdlib.h>

#include "msg_store.h"
#include "cache.h"

cache_policy_t g_cache_policy = CACHE_POLICY_RANDOM; //default to random

// helper func to print cache
static void print_cache_state(const char *condition) {
    printf("\n%s:\n", condition);
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_entry_t *entry = &g_cache.entries[i];
        if (entry->occupied) {
            printf("index %2d: id=%d last_used=%llu\n",
                   i,
                   entry->msg.content.id,
                   entry->last_used);
        } else {
            printf("index %2d: EMPTY\n", i);
        }
    }
    printf("\n");
}

/**
 * Basic test:
 * - initialize store and cache
 * - create and store a few messages
 * - retrieve one by ID and print it
 */
static void test_store_and_retrieve(void) {
    printf("\nTEST: Basic store + retrieve\n");

    if (init_msg_store() != 0) {
        fprintf(stderr, "test_store_and_retrieve: init_msg_store failed\n");
        return;
    }

    // init cache
    cache_init(&g_cache);
    g_cache_policy = CACHE_POLICY_RANDOM;

    // create test msgs
    message_t *m1 = create_msg("sender1", "recip1", "msg1");
    message_t *m2 = create_msg("sender2", "recip2", "msg2");
    message_t *m3 = create_msg("sender3", "recip3", "msg3");

    if (!m1 || !m2 || !m3) {
        fprintf(stderr, "test_store_and_retrieve: create_msg failed\n");
        free(m1); free(m2); free(m3);
        return;
    }

    // store messages (writes to both disk and cache)
    store_msg(m1);
    store_msg(m2);
    store_msg(m3);

    // retrieve 1 msg
    int target_id = m2->content.id;
    message_t *retrieved = retrieve_msg(target_id);
    if (retrieved) {
        printf("Retrieved message (id=%d):\n", target_id);
        printf("Timestamp: %s", ctime(&retrieved->content.timestamp));
        printf("Sender: %s\n", retrieved->content.sender);
        printf("Receiver: %s\n", retrieved->content.receiver);
        printf("Content: %s\n", retrieved->content.content);
        printf("Delivered: %s\n",
               retrieved->content.delivered ? "true" : "false");
        free(retrieved);
    } else {
        printf("Message with id=%d not found.\n", target_id);
    }

    print_cache_state("Cache after basic store + retrieve");

    // mem cleanup
    free(m1);
    free(m2);
    free(m3);
}

/**
 * Test RANDOM replacement:
 * - fill the cache with distinct IDs
 * - insert one more message
 * - verify that the new message is present and exactly one old one is gone
 */
static void test_random_replacement(void) {
    printf("\n[TEST] Random replacement\n");

    cache_init(&g_cache);
    g_cache_policy = CACHE_POLICY_RANDOM;

    // create CACHE_SIZE messages with IDs continuing from global_id
    message_t *msgs[CACHE_SIZE + 1];
    for (int i = 0; i < CACHE_SIZE + 1; i++) {
        char sender[32];
        char receiver[32];
        snprintf(sender, sizeof sender, "sender%d", i);
        snprintf(receiver, sizeof receiver, "receiver%d", i);

        msgs[i] = create_msg(sender, receiver, "random replacement test");
        if (!msgs[i]) {
            fprintf(stderr, "test_random_replacement: create_msg failed at i=%d\n", i);
            // free already created
            for (int j = 0; j < i; j++) free(msgs[j]);
            return;
        }
    }

    // insert first CACHE_SIZE messages into cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_insert(&g_cache, msgs[i], g_cache_policy);
    }

    print_cache_state("Cache after inserting CACHE_SIZE entries (RANDOM)");

    // insert one more; this should evict one of the existing entries
    cache_insert(&g_cache, msgs[CACHE_SIZE], g_cache_policy);

    print_cache_state("Cache after inserting one more entry (RANDOM)");

    // check that the new ID is present somewhere
    int new_id = msgs[CACHE_SIZE]->content.id;
    int found_new = 0;
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (g_cache.entries[i].occupied &&
            g_cache.entries[i].msg.content.id == new_id) {
            found_new = 1;
            break;
        }
    }
    if (!found_new) {
        printf("ERROR: new message id=%d not found in cache after RANDOM replacement\n", new_id);
    } else {
        printf("OK: new message id=%d is present after RANDOM replacement\n", new_id);
    }

    // cleanup
    for (int i = 0; i < CACHE_SIZE + 1; i++) {
        free(msgs[i]);
    }
}

/**
 * Test MRU (LIFO) replacement:
 * - fill the cache with distinct IDs
 * - access them in a known order so that the last one accessed is MRU
 * - insert a new message
 * - verify the MRU ID was evicted
 */
static void test_mru_replacement(void) {
    printf("\n[TEST] MRU (LIFO) replacement\n");

    cache_init(&g_cache);
    g_cache_policy = CACHE_POLICY_MRU;

    message_t *msgs[CACHE_SIZE + 1];
    for (int i = 0; i < CACHE_SIZE + 1; i++) {
        char sender[32];
        char receiver[32];
        snprintf(sender, sizeof sender, "mru_sender%d", i);
        snprintf(receiver, sizeof receiver, "mru_receiver%d", i);

        msgs[i] = create_msg(sender, receiver, "mru replacement test");
        if (!msgs[i]) {
            fprintf(stderr, "test_mru_replacement: create_msg failed at i=%d\n", i);
            for (int j = 0; j < i; j++) free(msgs[j]);
            return;
        }
    }

    // insert first CACHE_SIZE messages
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_insert(&g_cache, msgs[i], g_cache_policy);
    }

    // access them in order so the last one (index CACHE_SIZE-1) is MRU
    int mru_id = msgs[CACHE_SIZE - 1]->content.id;
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_lookup(&g_cache, msgs[i]->content.id);
    }

    print_cache_state("Cache before MRU replacement");

    // insert new message; MRU policy should evict the most recently used (mru_id)
    cache_insert(&g_cache, msgs[CACHE_SIZE], g_cache_policy);

    print_cache_state("Cache after MRU replacement");

    // verify that mru_id is no longer present
    int still_has_mru = 0;
    int new_id = msgs[CACHE_SIZE]->content.id;
    int has_new = 0;

    for (int i = 0; i < CACHE_SIZE; i++) {
        if (!g_cache.entries[i].occupied) continue;
        int id = g_cache.entries[i].msg.content.id;
        if (id == mru_id) still_has_mru = 1;
        if (id == new_id) has_new = 1;
    }

    if (still_has_mru) {
        printf("ERROR: MRU id=%d is still present after MRU replacement\n", mru_id);
    } else {
        printf("OK: MRU id=%d was evicted as expected\n", mru_id);
    }

    if (!has_new) {
        printf("ERROR: new message id=%d not found in cache after MRU replacement\n", new_id);
    } else {
        printf("OK: new message id=%d is present after MRU replacement\n", new_id);
    }

    // mem cleanup
    for (int i = 0; i < CACHE_SIZE + 1; i++) {
        free(msgs[i]);
    }
}

// main func to run tests and collect metrics
int main(void) {
    srand((unsigned int)time(NULL));

    // test cache
    test_store_and_retrieve();
    test_random_replacement();
    test_mru_replacement();

    // collect replacement algo metrics

    printf("\nProgram compelted successfully\n");
    return 0;
}

