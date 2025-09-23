cipher: cipher.o polybius.o
	clang -o cipher cipher.o polybius.o

cipher.o: cipher.c polybius.h
	clang -c cipher.c

polybius.o: polybius.c polybius.h
	clang -c polybius.c

clean:
	rm -f cipher cipher.o polybius.o