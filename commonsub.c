#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sais-lite-lcp/sais.h"

#define Constrain(expression) _Static_assert(expression, #expression)
Constrain(CHAR_BIT == 8);
#define InputMaxBytes (INT_MAX / 2 - 1)
#define MaxLen (InputMaxBytes + 1)

static unsigned char *Buf;
static int Len;
static int *SufArr;
static int *LongCommPre;
static int *Stack;

static void *AllocateOrDie(size_t count, size_t size) {
  void *ptr = calloc(count, size);
  if (ptr == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

static void Slurp(const char *filename) {
  FILE *stream = fopen(filename, "r");
  if (stream == NULL) goto fail;
  Len = fread(Buf, sizeof *Buf, MaxLen, stream);
  if (ferror(stream)) goto fail;
  if (Len == MaxLen) {
    fprintf(stderr, "%s: file is too large; increase InputMaxBytes\n",
            filename);
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < Len; i++) {
    if (Buf[i] < 0) {
      fprintf(stderr, "%s: file contains non-ASCII byte at offset %d\n",
              filename, i);
      exit(EXIT_FAILURE);
    }
  }
  if (fclose(stream) == EOF) goto fail;
  return;
fail:
  perror(filename);
  exit(EXIT_FAILURE);
}

static void FindCommonStrings(void) {
  Stack = AllocateOrDie(MaxLen, sizeof *Stack);
  int lenCommPre = -1;
  for (int i = 0; i < Len; i++) {
    while (lenCommPre > LongCommPre[i]) {
      int begin = Stack[lenCommPre];
      int count = i + 1 - begin;
      if (count > 1 && lenCommPre > 1) {
        printf("%d\t%.*s\n", count, lenCommPre, Buf + SufArr[begin]);
      }
      lenCommPre--;
    }
    while (lenCommPre < LongCommPre[i]) {
      lenCommPre++;
      Stack[lenCommPre] = i;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs("usage: commonsub input\n", stderr);
    exit(EXIT_FAILURE);
  }
  Buf = AllocateOrDie(MaxLen, sizeof *Buf);
  Len = 0;
  Slurp(argv[1]);
  Buf[Len] = UCHAR_MAX;
  Len++;
  SufArr = AllocateOrDie(MaxLen, sizeof *SufArr);
  LongCommPre = AllocateOrDie(MaxLen, sizeof *LongCommPre);
  sais(Buf, SufArr, LongCommPre, Len);
  if (false) {
    for (int i = 0; i < Len; i++) {
      printf("%d\t%d\t%d\t%.*s\n", i, SufArr[i], LongCommPre[i],
             Len - SufArr[i], Buf + SufArr[i]);
    }
  }
  FindCommonStrings();
}
