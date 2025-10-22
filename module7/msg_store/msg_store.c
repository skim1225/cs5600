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

int global_id = 0;

typedef struct {
    int id;
    time_t timestamp;
    char sender[50];
    char receiver[50];
    char content[1024];
    bool delivered;
} message_t;

// creates a new message with the fields appropriately set and returns a dynamically allocated message "object"
message_t* create_msg(const char* sender, const char* receiver, const char* content) {
    if (!sender || !receiver || !content) {
        fprintf(stderr, "Sender or receiver or content is null");
        return NULL;
    }

    // dynamically allocate msg obj
    message_t* msg = malloc(sizeof *msg);
    if (!msg) {
        fprintf(stderr, "Error allocating memory for message structure");
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
bool store_msg(message_t* msg) {

}

// finds and returns a message identified by its identifier
message_t* retrieve_msg(int id) {
}

int main() {
    return 0;
}