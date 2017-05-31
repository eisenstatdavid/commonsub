all: commonsub repsub
.PHONY: all

commonsub: commonsub.c sais-lite-lcp/sais.c sais-lite-lcp/sais.h Makefile
	$(CC) -Wall -o $@ commonsub.c sais-lite-lcp/sais.c -ffast-math -O3 -funroll-loops -DNDEBUG

repsub: repsub.c sais-lite-lcp/sais.c sais-lite-lcp/sais.h Makefile
	$(CC) -Wall -o $@ repsub.c sais-lite-lcp/sais.c -ffast-math -O3 -funroll-loops -DNDEBUG
