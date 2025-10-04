all: genWords multiprocessing

genWords: genWords-kim.c
	gcc genWords-kim.c -o genWords

multiprocessing: multiprocessing.c queue.c
	gcc multiprocessing.c queue.c -o multiprocessing

clean:
	rm -f genWords multiprocessing words.txt