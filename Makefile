commonsub: commonsub.c sais-lite-lcp/sais.c sais-lite-lcp/sais.h Makefile
	$(CC) -o $@ commonsub.c sais-lite-lcp/sais.c -ffast-math -O3 -funroll-loops -DNDEBUG
