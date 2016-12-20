#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sais-lite-lcp/sais.h"

#define Constrain(expression) _Static_assert(expression, #expression)
Constrain(CHAR_BIT == 8);
#define InputMaxBytes 100000000
Constrain(InputMaxBytes <= (INT_LEAST32_MAX - 2) / 2);
#define MaxLen (2 * InputMaxBytes + 2)
Constrain(MaxLen <= INT_FAST32_MAX / 2);

static unsigned char *Buf;
static int Len;
static int Begin2;
static int *SufArr;
static int *LongCommPre;
static unsigned long long *Bitmap2;
static int *SparseCount2;
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
  int n = fread(Buf + Len, sizeof *Buf, InputMaxBytes + 1, stream);
  if (ferror(stream)) goto fail;
  if (n > InputMaxBytes) {
    fprintf(stderr, "%s: file is too large; increase InputMaxBytes\n",
            filename);
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < n; i++) {
    if (Buf[Len + i] < 0) {
      fprintf(stderr, "%s: file contains non-ASCII byte at offset %d\n",
              filename, i);
      exit(EXIT_FAILURE);
    }
  }
  Len += n;
  if (fclose(stream) == EOF) goto fail;
  return;
fail:
  perror(filename);
  exit(EXIT_FAILURE);
}

static int PopCount(unsigned long long x) {
  int v = 0;
  while (x != 0) {
    x &= x - 1;
    v++;
  }
  return v;
}

static void BuildCumCount2(void) {
  Bitmap2 = AllocateOrDie((MaxLen >> 6) + 1, sizeof *Bitmap2);
  SparseCount2 = AllocateOrDie((MaxLen >> 6) + 1, sizeof *SparseCount2);
  for (int i = 0; i < Len; i++) {
    if (SufArr[i] >= Begin2) {
      Bitmap2[i >> 6] |= 1ULL << (i & 63);
      SparseCount2[i >> 6]++;
    }
  }
  for (int i = 0; i < (Len >> 6); i++) {
    SparseCount2[i + 1] += SparseCount2[i];
  }
}

static int CumCount2(int i) {
  return SparseCount2[i >> 6] - PopCount(Bitmap2[i >> 6] >> (i & 63));
}

static void FindCommonStrings(void) {
  Stack = AllocateOrDie(MaxLen, sizeof *Stack);
  int lenCommPre = -1;
  for (int i = 0; i < Len; i++) {
    while (lenCommPre > LongCommPre[i]) {
      int begin = Stack[lenCommPre];
      int end = i + 1;
      int count2 = CumCount2(end) - CumCount2(begin);
      if (count2 > 1 && count2 < end - begin && lenCommPre > 1) {
        printf("%d\t%.*s\n", count2, lenCommPre, Buf + SufArr[begin]);
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
  if (argc != 3) {
    fputs("usage: commonsub needle haystack\n", stderr);
    exit(EXIT_FAILURE);
  }
  Buf = AllocateOrDie(MaxLen, sizeof *Buf);
  Len = 0;
  Slurp(argv[1]);
  Buf[Len] = UCHAR_MAX - 1;
  Len++;
  Begin2 = Len;
  Slurp(argv[2]);
  Buf[Len] = UCHAR_MAX;
  SufArr = AllocateOrDie(MaxLen, sizeof *SufArr);
  LongCommPre = AllocateOrDie(MaxLen, sizeof *LongCommPre);
  sais(Buf, SufArr, LongCommPre, Len);
  if (false) {
    for (int i = 0; i < Len; i++) {
      printf("%d\t%d\t%d\t%.*s\n", i, SufArr[i], LongCommPre[i],
             (Len - SufArr[i]), Buf + SufArr[i]);
    }
  }
  BuildCumCount2();
  FindCommonStrings();
}
