all: genWords multiprocessing

genWords: genWords-kim.o
	gcc -o genWords genWords-kim.o

genWords-kim.o: genWords-kim.c
	gcc -c genWords-kim.c

multiprocessing: multiprocessing.o queue.o polybius.o
	gcc -o multiprocessing multiprocessing.o queue.o polybius.o

multiprocessing.o: multiprocessing.c queue.h polybius.h
	gcc -c multiprocessing.c

queue.o: queue.c queue.h
	gcc -c queue.c

polybius.o: polybius.c polybius.h
	gcc -c polybius.c

clean:
	rm -f genWords multiprocessing *.o encrypted*.txt words.txt