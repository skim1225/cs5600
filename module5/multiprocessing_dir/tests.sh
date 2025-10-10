#!/usr/bin/env bash

echo "Starting tests for multiprocessing..."

# Test 1: small input
echo "Running Test 1 - small input"
cp small.txt words.txt
./multiprocessing_dir
if [ -f encrypted.txt ]; then
    echo "Test 1 passed: encrypted.txt created"
else
    echo "Test 1 failed"
    exit 1
fi
rm -f words.txt encrypted*.txt
echo

# Test 2: multi-batch (>100 words)
echo "Running Test 2 - multi-batch input"
cp multi.txt words.txt
./multiprocessing_dir
if [ -f encrypted.txt ]; then
    echo "Test 2 passed: multi-batch handled correctly"
else
    echo "Test 2 failed"
    exit 1
fi
rm -f words.txt encrypted*.txt
echo

# Test 3: empty file
echo "Running Test 3 - empty input"
cp empty.txt words.txt
./multiprocessing_dir
if [ -f encrypted.txt ]; then
    echo "Test 3 passed: empty input handled"
else
    echo "Test 3 failed"
    exit 1
fi
rm -f words.txt encrypted*.txt
echo

# Test 4: missing input (no words.txt)
echo "Running Test 4 - missing input file"
rm -f words.txt
if ./multiprocessing_dir; then
    echo "Test 4 failed: should have exited with error"
    exit 1
else
    echo "Test 4 passed: missing file handled correctly"
fi
rm -f encrypted*.txt
echo

# Test 5: unique output naming
echo "Running Test 5 - unique output naming"
cp small.txt words.txt
./multiprocessing_dir
./multiprocessing_dir
if [ -f encrypted.txt ] && [ -f encrypted-1.txt ]; then
    echo "Test 5 passed: unique filenames created"
else
    echo "Test 5 failed"
    exit 1
fi
rm -f words.txt encrypted*.txt
echo

echo "All tests completed."
