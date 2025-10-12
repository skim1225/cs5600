/*
 * multithreading.c / Multithreading in C
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Oct 10, 2025
 *
 * Program which uses multithreading to encrypt words concurrently.
 */

// dependencies
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "queue.h"
#include "polybius.h"


// consts
#define MAX_LINE_LEN 256
#define BATCH_SIZE 100
#define MAX_BATCH_LEN 16384

static pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *out = NULL;

static const polybius_square_t square = {
    .square = {
        {'A','B','C','D','E'},
        {'F','G','H','I','K'},
        {'L','M','N','O','P'},
        {'Q','R','S','T','U'},
        {'V','W','X','Y','Z'}
    }
};

// util funcs

/**
 * @brief Frees all dynamically allocated strings stored in the queue.
 *
 * Iteratively pops each node from the queue and releases its associated memory.
 *
 * @param q Pointer to the queue to be cleaned up.
 */
static void cleanup(queue_t *q) {
    void *curr = popQ(q);
    while (curr != NULL) {
        free((char *) curr);
        curr = popQ(q);
    }
}

/**
 * @brief Thread routine that encrypts a batch of text and writes it to the output file.
 *
 * This function expects @p arg to be a heap-allocated, null-terminated string
 * containing a batch of newline-delimited words. It encodes the batch using the
 * Polybius cipher and appends the result to the shared output file, protecting the
 * write with a mutex to ensure thread-safe access.
 *
 * The function takes ownership of the input buffer and frees it. It also frees the
 * encrypted buffer produced by the cipher.
 *
 * @param arg Pointer to a dynamically-allocated C-string containing the batch to encrypt.
 * @return Always returns NULL (the return value is not used by the caller).
 *
 * @note Side effects: writes to the global @c out file handle while holding
 *       @c file_lock to ensure mutual exclusion.
 */
static void* thread_func(void* arg) {

	// encrypt string
	char *text_batch = (char *) arg;
	char* encrypted = pbEncode(text_batch, &square);
	if (encrypted == NULL) {
		free(text_batch);
		return NULL;
	}

	// lock file, write, and unlock
	pthread_mutex_lock(&file_lock);
	fputs(encrypted, out);
	fputc('\n', out);
	pthread_mutex_unlock(&file_lock);

	// cleanup
	free(encrypted);
	free(text_batch);
	return NULL;
}

/**
 * @brief Program entry point that orchestrates reading input, batching, threading, and cleanup.
 *
 * Workflow:
 *  - Reads words from "words.txt" and enqueues them.
 *  - Groups words into batches of size @c BATCH_SIZE.
 *  - Spawns one thread per batch to encrypt using the Polybius cipher.
 *  - Each thread writes its result to "out.txt" under a mutex.
 *  - Joins all threads and releases resources.
 *
 * Error handling is performed along the way; on failure, resources allocated up to
 * the failure point are released before returning a non-zero status code.
 *
 * @return 0 on success; non-zero on error (e.g., file I/O failure, allocation failure,
 *         queue insertion failure, or thread creation failure).
 */
int main(void) {

	/* Read the generated text file into your program and store each word
 	separately using the queue data structure you built in a prior assignment.
	*/
	FILE *fp;
	char line_buff[MAX_LINE_LEN];

	fp = fopen("words.txt", "r");

	if (fp == NULL) {
		perror("Error opening file");
		return 1;
	}

	// init q
	queue_t q = {NULL, NULL, 0};

	// write words to q
	while (fgets(line_buff, MAX_LINE_LEN, fp) != NULL) {
		// replace \n with terminator
		size_t len = strcspn(line_buff, "\r\n");
		line_buff[len] = '\0';

		// skip empty liens
		if (line_buff[0] == '\0') {
			continue;
		}

		// add word node to q
		char *word = strdup(line_buff);

		if (word == NULL) {
			perror("Error copying string");
			fclose(fp);
			cleanup(&q);
			return 1;
		}

		if (add2q(&q, word) != 0) {
			free(word);
			printf("Error adding word to queue\n");
			fclose(fp);
			cleanup(&q);
			return 1;
		}
	}

	fclose(fp);

	/* 1. (90 pts) Revisit the program from the prior assignment "Multiprocessing in C"
       and modify the program (after making a copy, of course), so that the encryptions
       are done concurrently by spawning a separate thread for the encryption of
       each batch of words. For this to work, you will need to compile your cipher
       code into this assignment rather than calling it as a separate program in
       a separate process. You do not need to construct the output in order.
	*/

	// gen outfile
	out = fopen("out.txt", "w");
	if (out == NULL) {
		perror("Error creating file");
		cleanup(&q);
		return 1;
	}

	// calc num batches
	int num_words = qsize(&q);
	int num_batches = (num_words + BATCH_SIZE - 1) / BATCH_SIZE;
	int num_threads = 0;

	// gen thread handle arr
	pthread_t* tids = (pthread_t *) malloc(sizeof(pthread_t) * (size_t) num_batches);
	if (tids == NULL) {
		perror("Error allocating threads");
		fclose(out);
		cleanup(&q);
		pthread_mutex_destroy(&file_lock);
		return 1;
	}

	char batch[MAX_BATCH_LEN];
	void *curr = popQ(&q);

	while (curr != NULL) {
		batch[0] = '\0';

		// concat 100 words in string at a time
		for (int i = 0; i < BATCH_SIZE && curr != NULL; i++) {
			strcat(batch, (char*) curr);
			strcat(batch, "\n");

			free(curr);
			curr = popQ(&q);
		}

		// skip empty
		if (batch[0] == '\0') {
			continue;
		}

		// create thread for each batch with copy of batch
		char* batch_copy = strdup(batch);
		if (batch_copy == NULL) {
			perror("Error duplicating batch");
			break;
		}

		int rc = pthread_create(&tids[num_threads], NULL, thread_func, batch_copy);
		if (rc != 0) {
			fprintf(stderr, "pthread_create failed: %s\n", strerror(rc));
			free(batch_copy);
			break;
		}
		num_threads++;
	}

	// join threads
	for (int i = 0; i < num_threads; i++) {
		pthread_join(tids[i], NULL);
	}

	// clean
	free(tids);
	fclose(out);
	cleanup(&q);
	pthread_mutex_destroy(&file_lock);
	return 0;

	/* 4. (10 pts) Using test cases of your own design, demonstrate that your
	 program works. Account for common edge conditions, such as a file without
	 sentences, one without termination markers (. ? !), where the cipher program
	 cannot be found, etc.
	*/

	// RUN tests.sh for test cases
}
