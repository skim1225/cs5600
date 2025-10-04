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
#include "queue.h"

#define MAX_LINE_LEN 256


int main() {

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

	while (fgets(line_buff, MAX_LINE_LEN, file_ptr) != NULL) {
		char *curr = strcspn(line_buff);
		// add word node to q
		add2q(&q, curr);
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

	/* 4. (10 pts) Using test cases of your own design, demonstrate that your
	 program works. Account for common edge conditions, such as a file without
	 sentences, one without termination markers (. ? !), where the cipher program
	 cannot be found, etc.
	*/

	// mem cleanup

	return 0;
}