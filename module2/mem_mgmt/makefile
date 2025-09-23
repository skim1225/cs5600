# default target
all: allocmem loadmem

allocmem: alloc-mem-kim.c
	clang -o allocmem alloc-mem-kim.c

loadmem: load-mem-kim.c
	clang -o loadmem load-mem-kim.c

clean:
	rm -f allocmem loadmem *.o