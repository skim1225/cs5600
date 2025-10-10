/*
 * multiprocessing.c / Multiprocessing in C
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Oct 2, 2025
 *
 * Program which uses multiprocessing to encrypt words concurrently.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "queue.h"
#include "polybius.h"

#define MAX_LINE_LEN 256
#define BATCH_SIZE 100
#define MAX_BATCH_LEN 16384

// helper func to clean up q nodes from mem
static void cleanup(queue_t *q) {
	void *curr = popQ(q);
	while (curr != NULL) {
		free((char *) curr);
		curr = popQ(q);
	}
}


// helper func to generate unique file names
static FILE *gen_filename(const char *base) {
	char name[128];
	int n = 0;
	int file_exists = 1;
	while (file_exists) {
		if (n == 0)
			snprintf(name, sizeof(name), "%s", base);
		else
			snprintf(name, sizeof(name), "%.*s-%d.txt",
					 (int)(strrchr(base, '.') ? strrchr(base, '.') - base : strlen(base)),
					 base, n);

		if (access(name, F_OK) != 0)
			file_exists = 0;

		n++;
	}

	FILE *fp = fopen(name, "w");
	if (!fp) {
		perror("fopen");
		return NULL;
	}

	fprintf(stderr, "Writing output to %s\n", name);
	return fp;
}


int main(void) {

	/* 2. (30 pts) Read the generated text file into your program and store each word
 	separately using the queue data structure you built in a prior assignment.
	*/
	FILE *file_ptr;
	char line_buff[MAX_LINE_LEN];

	file_ptr = fopen("words.txt", "r");

	if (file_ptr == NULL) {
		perror("Error opening file");
		return 1;
	}

	// init q
	queue_t q = {NULL, NULL, 0};

	// write words to q
	while (fgets(line_buff, MAX_LINE_LEN, file_ptr) != NULL) {
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
			fclose(file_ptr);
			cleanup(&q);
			return 1;
		}

		if (add2q(&q, word) != 0) {
			free(word);
			printf("Error adding word to queue\n");
			fclose(file_ptr);
			cleanup(&q);
			return 1;
		}
	}

	fclose(file_ptr);

	/* 3. 50 pts) Loop through the sentences and call your cipher program from a
	 prior assignment (as a separate process) to encrypt 100 words at a time --
	 you will likely need to modify your cipher program to take input differently
	 so you can pass multiple words at a time -- how you modify the passing of
	 words from the main program to a separate process is up to you, but recall
	 that the parent process and child process do not share common memory.
	 Investigate different strategies. Pipe (not write) the output of the cipher
	 program to a file in a common directory. Use some file naming convention or
	 mechanism that ensures that multiple cipher program invocations do not overwrite
	 a prior file. You may use any strategy you like. You do not need to construct
	 the output in order.
	*/

	// TODO: init square
	static const polybius_square_t square = {
		.square = {
			{'A', 'B', 'C', 'D', 'E'},
			{'F', 'G', 'H', 'I', 'K'},
			{'L', 'M', 'N', 'O', 'P'},
			{'Q', 'R', 'S', 'T', 'U'},
			{'V', 'W', 'X', 'Y', 'Z'}
		}
	};

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


	}


	/* 4. (10 pts) Using test cases of your own design, demonstrate that your
	 program works. Account for common edge conditions, such as a file without
	 sentences, one without termination markers (. ? !), where the cipher program
	 cannot be found, etc.
	*/


	// mem cleanup
	cleanup(&q);

	return 0;
}