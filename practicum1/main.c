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

/**
 * @brief Print the current state of the global cache.
 *
 * Iterates over all cache entries and prints either the message ID and
 * last_used value for occupied entries or "EMPTY" for unoccupied ones.
 *
 * @param condition Description of the current cache condition (e.g.,
 *                  "before test", "after replacement") to label the output.
 */
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
 * @brief Print a separator line for readability between test sections.
 */
static void print_line() {
    printf("----------------------------------------------------------\n");
}

/**
 * @brief Run a basic test of message storage and retrieval.
 *
 * Initializes the message store and cache, creates three test messages,
 * stores them (writing to both disk and cache), then retrieves one message
 * by ID. The function prints message details and the cache state, and
 * performs basic memory cleanup.
 */
static void test_store_and_retrieve(void) {
    printf("\nTEST: Basic store + retrieve\n\n");

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

    // store messages (write to both disk and cache)
    store_msg(m1);
    store_msg(m2);
    store_msg(m3);

    printf("Stored 3 messages to disk and cache.\n");

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
 * @brief Test the RANDOM cache replacement policy.
 *
 * Fills the cache with CACHE_SIZE messages, prints the cache state,
 * then inserts one more message to trigger random replacement. Verifies
 * that the new message is present in the cache afterward and reports
 * pass/fail, then frees all allocated messages.
 */
static void test_random_replacement(void) {
    printf("\nTEST: Random replacement\n");

    cache_init(&g_cache);
    g_cache_policy = CACHE_POLICY_RANDOM;

    // create CACHE_SIZE messages with IDs continuing from global_id
    message_t *msgs[CACHE_SIZE + 1];
    for (int i = 0; i < CACHE_SIZE + 1; i++) {

        msgs[i] = create_msg("random_sender", "random_receiver", "random replacement test");
        if (!msgs[i]) {
            fprintf(stderr, "test_random_replacement: create_msg failed at i=%d\n", i);
            // mem mgmt
            for (int j = 0; j < i; j++) free(msgs[j]);
            return;
        }
    }

    // fill cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_insert(&g_cache, msgs[i], g_cache_policy);
    }

    print_cache_state("Cache after inserting CACHE_SIZE entries (RANDOM)");

    // overfill cache
    cache_insert(&g_cache, msgs[CACHE_SIZE], g_cache_policy);

    print_cache_state("Cache after inserting one more entry (RANDOM)");

    // check new id replaced old id in cache
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
        printf("PASS: new message id=%d in cache after RANDOM replacement\n", new_id);
    }

    // cleanup
    for (int i = 0; i < CACHE_SIZE + 1; i++) {
        free(msgs[i]);
    }
}

/**
 * @brief Test the MRU (Most Recently Used) cache replacement policy.
 *
 * Fills the cache with CACHE_SIZE messages, then accesses each one in
 * order to establish a well-defined MRU entry. After inserting an extra
 * message to trigger MRU replacement, the function checks that the MRU
 * message was evicted, validates the presence of the new message, and
 * reports pass/fail before freeing all allocated messages.
 */
static void test_mru_replacement(void) {
    printf("\nTEST: Most Recently Used replacement\n");

    cache_init(&g_cache);
    g_cache_policy = CACHE_POLICY_MRU;

    message_t *msgs[CACHE_SIZE + 1];
    for (int i = 0; i < CACHE_SIZE + 1; i++) {

        msgs[i] = create_msg("mru_sender", "mru_receiver", "mru replacement test");
        if (!msgs[i]) {
            fprintf(stderr, "test_mru_replacement: create_msg failed at i=%d\n", i);
            for (int j = 0; j < i; j++) free(msgs[j]);
            return;
        }
    }

    // fill cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_insert(&g_cache, msgs[i], g_cache_policy);
    }

    // access all entries in order
    int mru_id = msgs[CACHE_SIZE - 1]->content.id;
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_lookup(&g_cache, msgs[i]->content.id);
    }

    print_cache_state("Cache before MRU replacement");

    // overfill cache, 16th entry should be replaced
    cache_insert(&g_cache, msgs[CACHE_SIZE], g_cache_policy);

    print_cache_state("Cache after MRU replacement");

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
        printf("PASS: MRU id=%d was replaced as expected\n", mru_id);
    }

    if (!has_new) {
        printf("ERROR: new message id=%d not found in cache after MRU replacement\n", new_id);
    } else {
        printf("PASS: new message id=%d is present after MRU replacement\n", new_id);
    }

    // mem cleanup
    for (int i = 0; i < CACHE_SIZE + 1; i++) {
        free(msgs[i]);
    }
}


/**
 * @brief Gather cache hit/miss metrics for a given replacement policy.
 *
 * Initializes the message store and cache, then creates and stores a fixed
 * set of messages. Performs a series of random message accesses via
 * retrieve_msg(), tracking global cache hit and miss counters. At the end,
 * prints the number of hits, misses, and the overall hit ratio for the
 * specified policy, then frees all allocated messages.
 *
 * @param policy Cache replacement policy (e.g., RANDOM or MRU) to evaluate.
 * @param name   Human-readable name for the policy, used in printed output.
 */
static void gather_metrics(cache_policy_t policy, const char *name) {

    const int NUM_MESSAGES = 32;
    const int NUM_ACCESSES = 1000;

    // reset store, cache, counters
    if (init_msg_store() != 0) {
        fprintf(stderr, "gather_metrics(%s): init_msg_store failed\n", name);
        return;
    }

    // re-init cache
    cache_init(&g_cache);
    g_cache_policy = policy;
    g_cache_hits = 0;
    g_cache_misses = 0;

    // create and store msg set for testing
    message_t *msgs[NUM_MESSAGES];
    for (int i = 0; i < NUM_MESSAGES; i++) {

        char sender[32];
        char receiver[32];
        snprintf(sender, sizeof sender, "user%d", i);
        snprintf(receiver, sizeof receiver, "dest%d", i);

        msgs[i] = create_msg(sender, receiver, "gather metrics msg");
        if (!msgs[i]) {
            fprintf(stderr, "gather_metrics(%s): create_msg failed at i=%d\n", name, i);
            // mem mgmt
            for (int j = 0; j < i; j++) {
                free(msgs[j]);
            }
            return;
        }

        // store_msg writes to disk and inserts into cache
        if (store_msg(msgs[i]) != 0) {
            fprintf(stderr, "gather_metrics(%s): store_msg failed at i=%d\n", name, i);
        }
    }

    // 1000 random message accesses
    for (int k = 0; k < NUM_ACCESSES; k++) {
        int index = rand() % NUM_MESSAGES;
        int id = msgs[index]->content.id;

        message_t *m = retrieve_msg(id);
        // mem mgmt
        if (m) {
            free(m);
        }
    }

    // calculate and show metrics
    unsigned long long total = g_cache_hits + g_cache_misses;
    double hit_ratio;
    if (total > 0) {
        hit_ratio = (double) g_cache_hits / (double) total;
    } else {
        hit_ratio = 0.0;
    }

    printf("\nMetrics for policy %s:\n", name);
    printf("Number of cache hits per 1000 random message accesses: %llu\n", g_cache_hits);   // since we did exactly 1000
    printf("Number of cache misses per 1000 random message accesses: %llu\n", g_cache_misses);
    printf("Cache hit ratio per 1000 random message accesses: %.3f\n", hit_ratio);

    // mem mgmt
    for (int i = 0; i < NUM_MESSAGES; i++) {
        free(msgs[i]);
    }
}


/**
 * @brief Program entry point for the cache and store simulation.
 *
 * Seeds the random number generator, runs a series of functional tests
 * for basic storage/retrieval and the RANDOM and MRU replacement policies,
 * then gathers and prints cache metrics for each policy. Finally, prints
 * a completion message and returns success.
 *
 * @return 0 on successful execution.
 */
int main(void) {
    srand((unsigned int)time(NULL));

    // PART 3: TEST CACHE FUNCS
    print_line();
    test_store_and_retrieve();
    print_line();
    test_random_replacement();
    print_line();
    test_mru_replacement();
    print_line();

    // PART 4: EVALUATE REPLACEMENT ALGORITHMS
    gather_metrics(CACHE_POLICY_RANDOM, "RANDOM");
    print_line();
    gather_metrics(CACHE_POLICY_MRU, "MRU");
    print_line();

    printf("\nProgram completed successfully\n");
    return 0;
}