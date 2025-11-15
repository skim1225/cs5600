/*
 * msg_store.h / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * Header file for the message store struct for practicum 1.
 */

#ifndef MSG_STORE_H
#define MSG_STORE_H

#include <time.h>
#include <stdbool.h>

#define MSG_SIZE 1024

typedef struct {
    int id;
    time_t timestamp;
    char sender[32];
    char receiver[32];
    char content[512];
    bool delivered;
} msg_content_t;

// wrapper to ensure fixed msg size
typedef struct {
    msg_content_t content;
    char padding[MSG_SIZE - sizeof(msg_content_t)];
} message_t;

int init_msg_store(void);
message_t *create_msg(const char *sender, const char *receiver, const char *content);
int store_msg(const message_t *msg);
message_t *retrieve_msg(int id);

#endif /* MSG_STORE_H */