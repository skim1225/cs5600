all: thread

thread: KimS.CS5600.F25.o
	clang -o thread KimS.CS5600.F25.o

KimS.CS5600.F25.o: KimS.CS5600.F25.c
	clang -c KimS.CS5600.F25.c

clean:
	rm -f thread KimS.CS5600.F25.o