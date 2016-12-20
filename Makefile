commonsub: commonsub.c sais-lite-lcp/sais.c sais-lite-lcp/sais.h
	sed -i~ /printf/d sais-lite-lcp/sais.c
	$(CC) -o $@ commonsub.c sais-lite-lcp/sais.c -O3 -fomit-frame-pointer
