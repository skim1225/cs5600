# Multiprocessing Encryption in C

Sooji Kim

10 October 2025

## Description:
This project demonstrates multiprocessing in C by encrypting words concurrently using the Polybius cipher.
It includes two main programs:
1. genWords which generates 10,000 random words and writes them to words.txt.
2. multiprocessing which reads words.txt, encrypts the words in batches of 100, and writes the results to uniquely named files such as encrypted.txt, encrypted-1.txt, etc.

## Build Instructions
To compile all programs, simply run:
```
make
```
This will build the following executables:
- genWords
- multiprocessing

## How to Run
Do 
```
./genWords
```
This creates a file named words.txt.

Run the multiprocessing encryption program:
```
./multiprocessing
```
This reads from words.txt, encrypts the words using child processes, and outputs encrypted text files:
1. encrypted.txt
2. encrypted-1.txt
...
 
## Testing
A basic test script tests.sh is included. Run
```
bash tests.sh
```
It will verify:
1. The existence of words.txt
2. That multiple runs of multiprocessing create uniquely named output files
 
## Cleanup
To remove all generated files and binaries, run:
```
make clean
```