# Bounded Buffer with Multithreading

This program demonstrates a bounded buffer shared by multiple producer threads.  
Each thread inserts strings of the form `ThreadID-RandomNumber` (e.g., `T2-48392`) into the buffer.  
It simulates IoT devices sending data to a shared buffer.

## Files
- `BoundedBuffer.java` — Thread-safe bounded buffer using locks and conditions
- `InsertStrings.java` — Main program that starts multiple threads and uses the buffer
- `Makefile` — Compiles, runs, cleans, and creates a submission archive

## How to Run
```bash
make        # Compile all Java files
make run    # Run the program
make clean  # Remove compiled files