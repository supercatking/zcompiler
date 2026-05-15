#include <stdint.h>
#include <stdio.h>

extern int vadd_i16(int16_t *c, int16_t *a, int16_t *b, int n);
extern int vadd_i16_m2(int16_t *c, int16_t *a, int16_t *b, int n);
extern int vadd_i16_m4(int16_t *c, int16_t *a, int16_t *b, int n);

#define CAPACITY 40

static int16_t wrap_i16(int value) { return (int16_t)value; }

static int run_one(int lmul, int n) {
  int16_t a[CAPACITY];
  int16_t b[CAPACITY];
  int16_t c[CAPACITY];
  int16_t sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    a[i] = wrap_i16((i % 2 == 0) ? (32000 - i * 17) : (-31000 + i * 19));
    b[i] = wrap_i16((i % 3 == 0) ? (1200 + i * 11) : (-2200 - i * 7));
    sentinel[i] = wrap_i16(-12345 - i);
    c[i] = sentinel[i];
  }

  int status = 0;
  if (lmul == 4)
    status = vadd_i16_m4(c, a, b, n);
  else if (lmul == 2)
    status = vadd_i16_m2(c, a, b, n);
  else
    status = vadd_i16(c, a, b, n);
  if (status != 0)
    return 1;

  for (int i = 0; i < n; ++i) {
    int16_t expected = wrap_i16((int)a[i] + (int)b[i]);
    if (c[i] != expected)
      return 10 + i;
  }
  for (int i = n; i < CAPACITY; ++i) {
    if (c[i] != sentinel[i])
      return 80 + i;
  }
  return 0;
}

int main(void) {
  int lengths[] = {0, 1, 2, 3, 5, 8, 17, 31};
  int count = sizeof(lengths) / sizeof(lengths[0]);
  for (int i = 0; i < count; ++i) {
    int status = run_one(0, lengths[i]);
    if (status != 0)
      return status;
    status = run_one(2, lengths[i]);
    if (status != 0)
      return 100 + status;
    status = run_one(4, lengths[i]);
    if (status != 0)
      return 200 + status;
  }
  printf("vector_add_i16 demo n=31 m1/m2/m4 passed\n");
  return 0;
}
