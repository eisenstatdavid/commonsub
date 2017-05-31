#include <ctype.h>
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
static int Set[SCHAR_MAX + 1];

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
    fprintf(stderr, "%s: file is too large\n", filename);
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < Len; i++) {
    if (Buf[i] > SCHAR_MAX) {
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

static void FindRepeatedStrings(void) {
  // findsmaxr from https://arxiv.org/pdf/1304.0528.pdf
  printf("[");
  bool needComma = false;
  int up = -1;
  for (int i = 1; i < Len; i++) {
    if (LongCommPre[i - 1] < LongCommPre[i]) {
      up = i;
      continue;
    }
    if (LongCommPre[i - 1] == LongCommPre[i] || up < 0) continue;
    for (int k = up - 1; k < i; k++) {
      if (SufArr[k] == 0) continue;
      unsigned char c = Buf[SufArr[k] - 1];
      if (Set[c] == i) goto skip;
      Set[c] = i;
    }
    if (needComma) {
      printf("\n,");
    }
    printf("\"");
    for (int j = 0; j < LongCommPre[up]; j++) {
      unsigned char c = Buf[SufArr[up] + j];
      if (iscntrl(c)) {
        printf("\\u%.4x", c);
      } else if (c == '\"' || c == '\\') {
        printf("\\%c", c);
      } else {
        printf("%c", c);
      }
    }
    printf("\"");
    needComma = true;
  skip:
    up = -1;
  }
  printf("\n]\n");
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs("usage: commonsub file\n", stderr);
    exit(EXIT_FAILURE);
  }
  Buf = AllocateOrDie(MaxLen, sizeof *Buf);
  Slurp(argv[1]);
  Buf[Len] = UCHAR_MAX;
  Len++;
  SufArr = AllocateOrDie(MaxLen, sizeof *SufArr);
  LongCommPre = AllocateOrDie(MaxLen, sizeof *LongCommPre);
  sais(Buf, SufArr, LongCommPre, Len);
  FindRepeatedStrings();
}
