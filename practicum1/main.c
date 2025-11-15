/*
 * main.c / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * This program simulates message caching, storage, and retrieval to explore
 * memory hierarchies and page replacement algorithms.
 */

#include <stdlib.h>
#include "msg_store.h"
#include "cache.h"

/**
 * @brief Main entry point for message store demonstration.
 *
 * This function demonstrates the functionality of initializing the
 * message store, creating and storing multiple messages, retrieving
 * one by ID, and printing its contents. It includes error handling
 * and cleanup of allocated memory.
 *
 * @return 0 on success, or 1 if initialization or allocation fails.
 */
int main() {

    // seed rand
    srand((unsigned int)time(NULL));

    // initialize msg store
    if (init_msg_store() != 0) {
        fprintf(stderr, "Failed to initialize message store.\n");
        return 1;
    }

    // init cache
    cache_t g_cache;

    // create some msgs
    message_t *m1 = create_msg("alice", "bob", "hi bob!!!");
    message_t *m2 = create_msg("carol", "dave", "hi dave from carol");
    message_t *m3 = create_msg("eve", "faith", "testing testing");

    // save some msgs to msg store
    store_msg(m1);
    store_msg(m2);
    store_msg(m3);

    // retrieve one msg and print it to verify
    int target_id = 2;
    message_t *retrieved = retrieve_msg(target_id);
    if (retrieved) {
        printf("\nRetrieved message (id=%d):\n", target_id);
        printf("  Timestamp: %s", ctime(&retrieved->timestamp));
        printf("  Sender:    %s\n", retrieved->sender);
        printf("  Receiver:  %s\n", retrieved->receiver);
        printf("  Content:   %s\n", retrieved->content);
        printf("  Delivered: %s\n", retrieved->delivered ? "true" : "false");
        free(retrieved);
    } else {
        printf("Message with id=%d not found.\n", target_id);
    }

    g_cache_policy = CACHE_POLICY_RANDOM;

    // RANDOM REPLACEMENT TEST
    g_cache_policy = CACHE_POLICY_RANDOM;
    // number of cache hits per 1000 random message accesses
    // number of cache misses per 1000 random message accesses
    // cache hit ratio per 1000 random message accesses

    // MRU REPLACEMENT TEST
    g_cache_policy = CACHE_POLICY_MRU;
    // number of cache hits per 1000 random message accesses
    // number of cache misses per 1000 random message accesses
    // cache hit ratio per 1000 random message accesses

    // cleanup
    // TODO: FREE() ALL CREATED MESSAGES

    printf("Program completed successfully.\n");
    return 0;
}
