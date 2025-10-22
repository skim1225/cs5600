/*
 * msg_store.c / Memory Hierarchy Simulation - Part I
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / October 22, 2025
 *
 * This program implements the beginning of a message-oriented data store.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define CSV_FILE "messages.csv"

int global_id = 0;

typedef struct {
    int id;
    time_t timestamp;
    char sender[50];
    char receiver[50];
    char content[1024];
    bool delivered;
} message_t;

// helper func to init msg store on disk
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

// creates a new message with the fields appropriately set and returns a dynamically allocated message "object"
message_t* create_msg(const char* sender, const char* receiver, const char* content) {
    if (!sender || !receiver || !content) {
        fprintf(stderr, "Sender or receiver or content is null\n");
        return NULL;
    }

    // dynamically allocate msg obj
    message_t* msg = malloc(sizeof *msg);
    if (!msg) {
        fprintf(stderr, "Error allocating memory for message structure\n");
        return NULL;
    }

    msg->id = global_id++;
    msg->timestamp = time(NULL);
    msg->delivered = false;

    strncpy(msg->sender, sender, sizeof(msg->sender) - 1);
    msg->sender[sizeof(msg->sender) - 1] = '\0';
    strncpy(msg->receiver, receiver, sizeof(msg->receiver) - 1);
    msg->receiver[sizeof(msg->receiver) - 1] = '\0';
    strncpy(msg->content, content, sizeof(msg->content) - 1);
    msg->content[sizeof(msg->content) - 1] = '\0';

    return msg;
}

// stores the message in a message store on disk
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

// finds and returns a message identified by its identifier
message_t* retrieve_msg(int id) {

    // input validation
    if (id > global
}

// test code, demonstrate funcs work as expected. check edge cases and error handling
int main() {
    init_msg_store();
    return 0;
}