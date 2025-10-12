# Multithreading in C
Sooji Kim
11 October 2025

## Description
This program demonstrates multithreading in C using the POSIX threads (pthreads) library.
It revisits the Multiprocessing in C assignment but replaces processes with threads. Each thread encrypts a batch of up to 100 words using the Polybius cipher and writes its output to a shared file, synchronized with a mutex lock.
The program showcases how threads share memory space and can work concurrently to process large input data efficiently.

## Compilation
To compile the program, navigate to the appropriate directory in the terminal and run
```angular2html
make
```
To produce an executable named multithreading.

## Running the Program
Make sure you have an input file named words.txt in the same directory.
Each line should contain one word to encrypt.
Run the program in the terminal:
```
./multithreading
```
The encrypted output will be saved to:
out.txt


## Running Tests
Verify the program works on various cases by running the provided test script.

In the terminal run:
```angular2html
chmod +x tests.sh
./tests.sh
```
This script runs the program on multiple input files (e.g., basic, empty, whitespace, etc.)
and saves each result in the outputs/ directory.

## Cleanup
To remove compiled files and outputs, run:
```
make clean
```