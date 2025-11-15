/*
 * msg_store.c / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * This file contains creation, storage, and retrieval functions for the message store object.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "msg_store.h"
#include "cache.h"

#define CSV_FILE "messages.csv"

int global_id = 1;
cache_t g_cache;

unsigned long long g_cache_hits = 0;
unsigned long long g_cache_misses = 0;

extern cache_policy_t g_cache_policy;

/**
 * @brief Initialize the message store on disk.
 *
 * This function creates or overwrites the CSV file used as the message store,
 * writes the header row, and closes the file. It ensures the message store
 * is ready for subsequent read/write operations.
 *
 * @return 0 on success, or 1 if an error occurred while opening, writing,
 *         or closing the file.
 */
int init_msg_store() {

    // open file
    FILE* fp;
    fp = fopen(CSV_FILE, "w");
    if (fp == NULL) {
        perror("init_msg_store: Error opening file");
        return 1;
    }

    // write headers
    if (fprintf(fp, "id,timestamp,sender,receiver,content,delivered\n") < 0) {
        perror("init_msg_store: Error writing to file");
        fclose(fp);
        return 1;
    }

    // close file
    if (fclose(fp) != 0) {
        perror("init_msg_store: Error closing file");
        return 1;
    }

    return 0;
}

/**
 * @brief Create a new message object dynamically.
 *
 * This function allocates and initializes a new message_t structure with
 * the provided sender, receiver, and content. It assigns a unique ID and
 * timestamp automatically.
 *
 * @param sender Pointer to the sender's name string.
 * @param receiver Pointer to the receiver's name string.
 * @param content Pointer to the message content string.
 * @return Pointer to a newly allocated message_t object, or NULL if
 *         allocation fails or any parameter is NULL.
 */
message_t* create_msg(const char* sender, const char* receiver, const char* content) {
    if (!sender || !receiver || !content) {
        fprintf(stderr, "create_msg: Sender or receiver or content is null\n");
        return NULL;
    }

    message_t *msg = malloc(sizeof *msg);
    if (!msg) {
        fprintf(stderr, "create_msg: Error allocating memory for message structure\n");
        return NULL;
    }

    msg->content.id = global_id++;
    msg->content.timestamp = time(NULL);
    msg->content.delivered = false;

    strncpy(msg->content.sender, sender, sizeof(msg->content.sender) - 1);
    msg->content.sender[sizeof(msg->content.sender) - 1] = '\0';

    strncpy(msg->content.receiver, receiver, sizeof(msg->content.receiver) - 1);
    msg->content.receiver[sizeof(msg->content.receiver) - 1] = '\0';

    strncpy(msg->content.content, content, sizeof(msg->content.content) - 1);
    msg->content.content[sizeof(msg->content.content) - 1] = '\0';

    return msg;
}

/**
 * @brief Store a message to the CSV-based message store.
 *
 * This function appends a message record to the existing CSV file.
 * The message is serialized into a comma-separated line containing
 * its ID, timestamp, sender, receiver, content, and delivery status.
 *
 * @param msg Pointer to the message_t object to be stored.
 * @return 0 on success, or 1 if an error occurred while opening,
 *         writing, or closing the file.
 */
int store_msg(const message_t* msg) {

    // input validation
    if (msg == NULL) {
        fprintf(stderr, "store_msg: Message is null\n");
        return 1;
    }

    // open file
    FILE* fp;
    fp = fopen(CSV_FILE, "a");
    if (fp == NULL) {
        perror("store_msg: Error opening file");
        return 1;
    }

    long long ts = (long long) msg->content.timestamp;

    // write msg to file
    if (fprintf(fp, "%d,%lld,%s,%s,%s,%s\n",
            msg->content.id,
            ts,
            msg->content.sender,
            msg->content.receiver,
            msg->content.content,
            msg->content.delivered ? "true" : "false") < 0) {
        perror("store_msg: Error writing to file");
        fclose(fp);
        return 1;
    }

    // close file
    if (fclose(fp) != 0) {
        perror("store_msg: Error closing file");
        return 1;
    }

    // insert msg into cache
    cache_insert(&g_cache, msg, g_cache_policy);

    return 0;
}

/**
 * @brief Retrieve a stored message by its unique identifier.
 *
 * This function first searches the cache for a message with the specified ID.
 * If not in cache, it then searches the CSV message store. If found, it constructs
 * and returns a dynamically allocated message_t object populated with the stored data.
 *
 * @param id Integer identifier of the message to retrieve.
 * @return Pointer to a newly allocated message_t if found, or NULL
 *         if the message does not exist or an error occurs.
 */
message_t* retrieve_msg(int id) {

    // input validation
    if (id < 0 || id > global_id) {
        fprintf(stderr, "retrieve_msg: invalid id\n");
        return NULL;
    }

    // check in cache for msg
    message_t *cached = cache_lookup(&g_cache, id);
    if (cached) {

        // cache hit counter
        g_cache_hits++;

        message_t *result = malloc(sizeof *result);
        if (!result) {
            fprintf(stderr, "retrieve_msg: malloc failed (cache copy)\n");
            return NULL;
        }
        *result = *cached;
        return result;
    }

    // cache miss counter
    g_cache_misses++;

    // check disk for msg
    FILE* fp = fopen(CSV_FILE, "r");
    if (!fp) {
        perror("retrieve_msg: Error opening file");
        return NULL;
    }

    char line_buf[2048];

    // skip header
    if (!fgets(line_buf, sizeof line_buf, fp)) {
        fclose(fp);
        return NULL;
    }

    // iterate over csv rows
    while (fgets(line_buf, sizeof line_buf, fp) != NULL) {
        size_t n = strlen(line_buf);
        while (n > 0 && (line_buf[n-1] == '\n' || line_buf[n-1] == '\r')) {
            line_buf[--n] = '\0';
        }

        // tokenize csv line
        char *tok_id = strtok(line_buf, ",");
        char *tok_ts = strtok(NULL, ",");
        char *tok_sender = strtok(NULL, ",");
        char *tok_receiver = strtok(NULL, ",");
        char *tok_content = strtok(NULL, ",");
        char *tok_deliv = strtok(NULL, ",");

        if (!tok_id || !tok_ts || !tok_sender || !tok_receiver || !tok_content || !tok_deliv) {
            continue;
        }

        int parsed_id = (int)strtol(tok_id, NULL, 10);
        if (parsed_id != id) {
            continue;
        }

        // rebuild msg
        message_t *msg = malloc(sizeof *msg);
        if (!msg) {
            fprintf(stderr, "retrieve_msg: malloc failed\n");
            fclose(fp);
            return NULL;
        }

        msg->content.id = parsed_id;
        msg->content.timestamp = (time_t)strtoll(tok_ts, NULL, 10);
        msg->content.delivered = (strcmp(tok_deliv, "true") == 0);

        strncpy(msg->content.sender, tok_sender, sizeof msg->content.sender - 1);
        strncpy(msg->content.receiver, tok_receiver, sizeof msg->content.receiver - 1);
        strncpy(msg->content.content, tok_content, sizeof msg->content.content - 1);

        msg->content.sender[sizeof msg->content.sender - 1] = '\0';
        msg->content.receiver[sizeof msg->content.receiver - 1] = '\0';
        msg->content.content[sizeof msg->content.content - 1] = '\0';

        fclose(fp);

        // add msg found in disk to cache
        cache_insert(&g_cache, msg, g_cache_policy);

        return msg;
    }

    fclose(fp);

    // msg not found
    return NULL;
}