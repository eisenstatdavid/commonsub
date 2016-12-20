commonsub: commonsub.c sais-lite-lcp/sais.c sais-lite-lcp/sais.h
	$(CC) -o $@ commonsub.c sais-lite-lcp/sais.c -O3 -fomit-frame-pointer
