all: main

main: main.o msg_store.o cache.o
	gcc main.o msg_store.o cache.o -o main

main.o: main.c msg_store.h cache.h message.h
	gcc -c main.c

msg_store.o: msg_store.c msg_store.h cache.h message.h
	gcc -c msg_store.c

cache.o: cache.c cache.h message.h
	gcc -c cache.c

clean:
	rm -f *.o main
