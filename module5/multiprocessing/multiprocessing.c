/*
 * multiprocessing.c / Multiprocessing in C
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Oct 2, 2025
 *
 * TODO
 */


int main() {
	/* 1. (10 pts) Revisit the program from the prior assignment "Random Number Generation"
 	and have it generate small text strings (words) instead of numbers and write them to a file.
 	The file must contain at least 10,000 words.
	*/

	/* 2. (30 pts) Read the generated text file into your program and store each word
 	separately using the queue data structure you built in a prior assignment.
	*/

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

	return 0;
}