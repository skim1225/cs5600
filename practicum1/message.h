/*
 * message.h / Practicum 1
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 25 / November 13, 2025
 *
 * Header file for message object
 */

#ifndef MESSAGE_H
#define MESSAGE_H

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

typedef struct message_t {
    msg_content_t content;
    char padding[MSG_SIZE - sizeof(msg_content_t)];
} message_t;

#endif