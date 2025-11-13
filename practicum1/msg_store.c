/*
 * msg_store.c / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * This program implements the beginning of a message-oriented data store.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define CSV_FILE "messages.csv"
#define MSG_SIZE 1024
#define CACHE_SIZE 16

int global_id = 1;

typedef struct {
    int id;
    time_t timestamp;
    char sender[32];
    char receiver[32];
    char content[512];
    bool delivered;
} msg_content_t;

typedef struct {
    msg_content_t content;
    char padding[MSG_SIZE - sizeof(msg_content_t)];
} message_t;

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
        perror("Error opening file");
        return 1;
    }

    // write headers
    if (fprintf(fp, "id,timestamp,sender,receiver,content,delivered\n") < 0) {
        perror("Error writing to file");
        fclose(fp);
        return 1;
    }

    // close file
    if (fclose(fp) != 0) {
        perror("Error closing file");
        return 1;
    }

    printf("Message store initiated.\n");
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
        fprintf(stderr, "Sender or receiver or content is null\n");
        return NULL;
    }

    message_t *msg = malloc(sizeof *msg);
    if (!msg) {
        fprintf(stderr, "Error allocating memory for message structure\n");
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
        fprintf(stderr, "Message is null\n");
        return 1;
    }

    // open file
    FILE* fp;
    fp = fopen(CSV_FILE, "a");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    long long ts = (long long) msg->timestamp;

    // write msg to file
    if (fprintf(fp, "%d,%lld,%s,%s,%s,%s\n",
            msg->id,
            ts,
            msg->sender,
            msg->receiver,
            msg->content,
            msg->delivered ? "true" : "false") < 0) {
        perror("Error writing to file");
        fclose(fp);
        return 1;
    }

    // close file
    if (fclose(fp) != 0) {
        perror("Error closing file");
        return 1;
    }

    printf("Message added to store.\n");
    return 0;
}

/**
 * @brief Retrieve a stored message by its unique identifier.
 *
 * This function searches the CSV message store for a message with
 * the specified ID. If found, it constructs and returns a dynamically
 * allocated message_t object populated with the stored data.
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

    FILE* fp = fopen(CSV_FILE, "r");
    if (!fp) {
        perror("Error opening file");
        return NULL;
    }

    char line_buf[2048];

    // skip header
    if (!fgets(line_buf, sizeof line_buf, fp)) {
        fclose(fp);
        return NULL;
    }

    // iterate over rows of csv
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

        // check if tokenizing worked
        if (!tok_id || !tok_ts || !tok_sender || !tok_receiver || !tok_content || !tok_deliv) {
            continue;
        }

        int parsed_id = (int)strtol(tok_id, NULL, 10);
        if (parsed_id != id) {
            continue;
        }

        // Build result
        message_t *msg = (message_t *)malloc(sizeof *msg);
        if (!msg) {
            fprintf(stderr, "retrieve_msg: malloc failed\n");
            fclose(fp);
            return NULL;
        }

        msg->id = parsed_id;
        msg->timestamp = (time_t)strtoll(tok_ts, NULL, 10);
        msg->delivered = (strcmp(tok_deliv, "true") == 0);

        strncpy(msg->sender,   tok_sender,   sizeof msg->sender   - 1);
        msg->sender[sizeof msg->sender - 1] = '\0';
        strncpy(msg->receiver, tok_receiver, sizeof msg->receiver - 1);
        msg->receiver[sizeof msg->receiver - 1] = '\0';
        strncpy(msg->content,  tok_content,  sizeof msg->content  - 1);
        msg->content[sizeof msg->content - 1] = '\0';

        fclose(fp);
        return msg;
    }

    // msg not found
    fclose(fp);
    return NULL;
}

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

    // initialize msg store
    if (init_msg_store() != 0) {
        fprintf(stderr, "Failed to initialize message store.\n");
        return 1;
    }

    // create some msgs
    message_t *m1 = create_msg("alice", "bob", "hi bob!!!");
    message_t *m2 = create_msg("carol", "dave", "hi dave from carol");
    message_t *m3 = create_msg("eve", "faith", "testing testing");

    if (!m1 || !m2 || !m3) {
        fprintf(stderr, "Error creating messages.\n");
        free(m1);
        free(m2);
        free(m3);
        return 1;
    }

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

    // cleanup
    free(m1);
    free(m2);
    free(m3);

    printf("Program completed successfully.\n");
    return 0;
}
