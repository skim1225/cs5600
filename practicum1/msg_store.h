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

#include "message.h"
#include "cache.h"

extern cache_t g_cache;
extern cache_policy_t g_cache_policy;
extern unsigned long long g_cache_hits;
extern unsigned long long g_cache_misses;

int init_msg_store(void);
message_t *create_msg(const char *sender, const char *receiver, const char *content);
int store_msg(const message_t *msg);
message_t *retrieve_msg(int id);

#endif /* MSG_STORE_H */