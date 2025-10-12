#!/usr/bin/env bash

mkdir -p outputs

run_one () {
  local infile="$1"
  local tag="$2"
  echo "Running test: $tag"
  cp "inputs/$infile" words.txt
  ./multithreading
  mv -f out.txt "outputs/out_${tag}.txt"
  echo "Wrote outputs/out_${tag}.txt"
  echo
}

run_one "test_basic.txt" "basic"
run_one "test_empty.txt" "empty"
run_one "test_whitespace.txt" "whitespace"
run_one "test_100_words.txt" "100_words"
run_one "test_101_words.txt" "101_words"
run_one "test_mixed_punct.txt" "mixed_punct"