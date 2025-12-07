server: server.c
	gcc -o server server.c -pthread

rfs: rfs.c
	gcc -o rfs rfs.c

all: server rfs

clean:
	rm -f server rfs