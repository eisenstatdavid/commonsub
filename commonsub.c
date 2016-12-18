#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef int_fast32_t I32;

#define Constrain(expression) _Static_assert(expression, #expression)
Constrain(CHAR_BIT == 8);
#define InputMaxBytes 80000000
Constrain(InputMaxBytes <= (INT_LEAST32_MAX - 2) / 2);
#define MaxLen (2 * InputMaxBytes + 2)
Constrain(MaxLen <= INT_FAST32_MAX / 2);

static I32 Len;
static I32 Begin2;
static signed char Buf[MaxLen];
static int_least32_t SufArr[MaxLen];
static int_least32_t SufRank[MaxLen];
static int_least32_t NewRank[MaxLen];
static int_least32_t *const LongCommPre = NewRank;  // aliased to save space
static uint_least64_t Bitmap2[(MaxLen >> 6) + 1];
static int_least32_t SparseCount2[(MaxLen >> 6) + 1];
static int_least32_t *const Stack = SufRank;  // aliased to save space

static void Slurp(const char *filename) {
  FILE *stream = fopen(filename, "r");
  if (stream == NULL) goto fail;
  I32 n = fread(Buf + Len, sizeof *Buf, InputMaxBytes + 1, stream);
  if (ferror(stream)) goto fail;
  if (n > InputMaxBytes) {
    fprintf(stderr, "%s: file is too large; increase InputMaxBytes\n",
            filename);
    exit(EXIT_FAILURE);
  }
  for (I32 i = 0; i < n; i++) {
    if (Buf[Len + i] < 0) {
      fprintf(stderr,
              "%s: file contains non-ASCII byte at offset %" PRIdFAST32 "\n",
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

static I32 Radix;

static int CompareRankPairs(const void *iPtr, const void *jPtr) {
  I32 i = *(const int_least32_t *)iPtr;
  I32 j = *(const int_least32_t *)jPtr;
  if (SufRank[i] < SufRank[j]) return -1;
  if (SufRank[i] > SufRank[j]) return 1;
  I32 iRank = i + Radix < Len ? SufRank[i + Radix] : -2;
  I32 jRank = j + Radix < Len ? SufRank[j + Radix] : -2;
  if (iRank < jRank) return -1;
  if (iRank > jRank) return 1;
  return 0;
}

static void BuildSuffixArray(void) {
  for (I32 i = 0; i < Len; i++) {
    SufArr[i] = i;
    SufRank[i] = Buf[i];
  }
  for (Radix = 1; true; Radix *= 2) {
    qsort(SufArr, Len, sizeof *SufArr, CompareRankPairs);
    NewRank[0] = 0;
    for (I32 i = 1; i < Len; i++) {
      NewRank[i] = CompareRankPairs(&SufArr[i - 1], &SufArr[i]) == 0
                       ? NewRank[i - 1]
                       : NewRank[i - 1] + 1;
    }
    for (I32 i = 0; i < Len; i++) {
      SufRank[SufArr[i]] = NewRank[i];
    }
    if (NewRank[Len - 1] == Len - 1) break;
  }

  I32 lenCommPre = 0;
  for (I32 i = 0; i < Len; i++) {
    if (SufRank[i] == Len - 1) {
      LongCommPre[SufRank[i]] = -1;
      continue;
    }
    while (Buf[i + lenCommPre] == Buf[SufArr[SufRank[i] + 1] + lenCommPre]) {
      lenCommPre++;
    }
    LongCommPre[SufRank[i]] = lenCommPre;
    if (lenCommPre > 0) lenCommPre--;
  }
}

static I32 PopCount(uint_fast64_t x) {
  I32 v = 0;
  while (x != 0) {
    x &= x - 1;
    v++;
  }
  return v;
}

static void BuildCumCount2(void) {
  for (I32 i = 0; i < Len; i++) {
    if (SufArr[i] >= Begin2) {
      Bitmap2[i >> 6] |= UINT64_C(1) << (i & 63);
      SparseCount2[i >> 6]++;
    }
  }
  for (I32 i = 0; i < (Len >> 6); i++) {
    SparseCount2[i + 1] += SparseCount2[i];
  }
}

static I32 CumCount2(I32 i) {
  return SparseCount2[i >> 6] - PopCount(Bitmap2[i >> 6] >> (i & 63));
}

static void FindCommonStrings(void) {
  I32 lenCommPre = -1;
  for (I32 i = 0; i < Len; i++) {
    while (lenCommPre > LongCommPre[i]) {
      I32 begin = Stack[lenCommPre];
      I32 end = i + 1;
      I32 count2 = CumCount2(end) - CumCount2(begin);
      if (count2 > 0 && count2 < end - begin && lenCommPre > 0) {
        printf("%" PRIdFAST32 "\t%.*s\n", count2, (int)lenCommPre,
               Buf + SufArr[begin]);
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
  Len = 0;
  Slurp(argv[1]);
  Buf[Len] = -1;
  Len++;
  Begin2 = Len;
  Slurp(argv[2]);
  Buf[Len] = -2;  // sentinel
  BuildSuffixArray();
  if (false) {
    for (I32 i = 0; i < Len; i++) {
      printf("%" PRIdFAST32 "\t%" PRIdLEAST32 "\t%" PRIdLEAST32 "\t%.*s\n", i,
             SufArr[i], LongCommPre[i], (int)(Len - SufArr[i]),
             Buf + SufArr[i]);
    }
  }
  BuildCumCount2();
  FindCommonStrings();
}
