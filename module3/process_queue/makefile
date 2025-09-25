all: qtest

qtest: queue.o queuetest.o
	clang -o qtest queue.o queuetest.o

queue.o: queue.c queue.h
	clang -c queue.c

queuetest.o: queuetest.c queue.h
	clang -c queuetest.c

clean:
	rm -f queue.o queuetest.o qtest
