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

static void *thread_encrypt_func(void* arg) {
    thread_job_t *job = (thread_job_t *) arg;

}

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
	FILE *out = fopen("out.txt", "w");
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
		pthread_mutx_destroy(&file_lock);
		return 1;
	}


	char batch[MAX_BATCH_LEN];
	int count = 0;
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

		// create pipe + validate
		int pfd[2];
		if (pipe(pfd) != 0) {
			perror("Error creating pipe");
			cleanup(&q);
			fclose(out);
			return 1;
		}

		// create child process + validate
		pid_t pid = fork();
		if (pid < 0) {
			perror("Error creating child process");
			close(pfd[0]);
			close(pfd[1]);
			cleanup(&q);
			fclose(out);
			return 1;
		}

		// child process for encryption
		if (pid == 0) {
			close(pfd[0]);
			char* encrypted = pbEncode(batch, &square);
			if (!encrypted) {
				close(pfd[1]);
				_exit(1);
			}
			// pipe to parent
			dprintf(pfd[1], "%s\n", encrypted);

			free(encrypted);
			close(pfd[1]);
			_exit(0);
		}

		// parent write to file
		close(pfd[1]);
		FILE *pipe_read = fdopen(pfd[0], "r");
		if (pipe_read == NULL) {
			perror("Error opening pipe read file");
			(void) waitpid(pid, NULL, 0);
			fclose(out);
			cleanup(&q);
			return 1;
		}

		char buf[4096];
		size_t bytes_read;

		while (!feof(pipe_read)) {
			bytes_read = fread(buf, 1, sizeof(buf), pipe_read);
			if (bytes_read > 0) {
				fwrite(buf, 1, bytes_read, out);
			}
			if (ferror(pipe_read)) {
				perror("Error reading from pipe");
				break;
			}
		}

		fclose(pipe_read);
		(void) waitpid(pid, NULL, 0);
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
