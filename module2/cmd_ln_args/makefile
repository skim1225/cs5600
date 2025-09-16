arger-kim: arger-kim.o arger-utils.o
	clang -o arger-kim arger-kim.o arger-utils.o

arger-kim.o: arger-kim.c arger-utils.h
	clang -c arger-kim.c

arger-utils.o: arger-utils.c arger-utils.h
	clang -c arger-utils.c

clean:
	rm -f arger-kim arger-kim.o arger-utils.o
