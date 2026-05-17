#include <stdint.h>
#include <stdio.h>

extern int vadd_i16(int16_t *c, int16_t *a, int16_t *b, int n);
extern int vadd_i16_m2(int16_t *c, int16_t *a, int16_t *b, int n);
extern int vadd_i16_m4(int16_t *c, int16_t *a, int16_t *b, int n);
extern int vadd_i8(int8_t *c, int8_t *a, int8_t *b, int n);
extern int vadd_i64(int64_t *c, int64_t *a, int64_t *b, int n);
extern int vcopy_i8(int8_t *out, int8_t *input, int n);
extern int vcopy_i64(int64_t *out, int64_t *input, int n);
extern int select_gt_i8(int8_t *lhs, int8_t *rhs, int8_t *true_values,
                        int8_t *false_values, int8_t *out, int n);
extern int select_gt_i64(int64_t *lhs, int64_t *rhs, int64_t *true_values,
                         int64_t *false_values, int64_t *out, int n);

#define CAPACITY 40

static int16_t wrap_i16(int value) { return (int16_t)value; }
static int8_t wrap_i8(int value) { return (int8_t)(uint8_t)value; }

static int run_i16_add_one(int lmul, int n) {
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

static int run_i8_add_one(int n) {
  int8_t a[CAPACITY];
  int8_t b[CAPACITY];
  int8_t c[CAPACITY];
  int8_t sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    a[i] = wrap_i8((i % 2 == 0) ? (120 - i * 5) : (-110 + i * 3));
    b[i] = wrap_i8((i % 3 == 0) ? (30 + i * 2) : (-50 - i));
    sentinel[i] = wrap_i8(-77 - i);
    c[i] = sentinel[i];
  }

  if (vadd_i8(c, a, b, n) != 0)
    return 1;
  for (int i = 0; i < n; ++i) {
    int8_t expected = wrap_i8((int)a[i] + (int)b[i]);
    if (c[i] != expected)
      return 10 + i;
  }
  for (int i = n; i < CAPACITY; ++i) {
    if (c[i] != sentinel[i])
      return 80 + i;
  }
  return 0;
}

static int run_i64_add_one(int n) {
  int64_t a[CAPACITY];
  int64_t b[CAPACITY];
  int64_t c[CAPACITY];
  int64_t sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    a[i] = (int64_t)10000000000LL + i * 101;
    b[i] = (i % 2 == 0) ? (int64_t)(-7000000000LL + i * 11)
                        : (int64_t)(3000000000LL - i * 13);
    sentinel[i] = -900000000000LL - i;
    c[i] = sentinel[i];
  }

  if (vadd_i64(c, a, b, n) != 0)
    return 1;
  for (int i = 0; i < n; ++i) {
    int64_t expected = a[i] + b[i];
    if (c[i] != expected)
      return 10 + i;
  }
  for (int i = n; i < CAPACITY; ++i) {
    if (c[i] != sentinel[i])
      return 80 + i;
  }
  return 0;
}

static int run_i8_copy_one(int n) {
  int8_t input[CAPACITY];
  int8_t out[CAPACITY];
  int8_t sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    input[i] = wrap_i8(45 - i * 4);
    sentinel[i] = wrap_i8(-99 + i);
    out[i] = sentinel[i];
  }

  if (vcopy_i8(out, input, n) != 0)
    return 1;
  for (int i = 0; i < n; ++i) {
    if (out[i] != input[i])
      return 10 + i;
  }
  for (int i = n; i < CAPACITY; ++i) {
    if (out[i] != sentinel[i])
      return 80 + i;
  }
  return 0;
}

static int run_i64_copy_one(int n) {
  int64_t input[CAPACITY];
  int64_t out[CAPACITY];
  int64_t sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    input[i] = -50000000000LL + i * 4099;
    sentinel[i] = 60000000000LL + i;
    out[i] = sentinel[i];
  }

  if (vcopy_i64(out, input, n) != 0)
    return 1;
  for (int i = 0; i < n; ++i) {
    if (out[i] != input[i])
      return 10 + i;
  }
  for (int i = n; i < CAPACITY; ++i) {
    if (out[i] != sentinel[i])
      return 80 + i;
  }
  return 0;
}

static int run_i8_select_one(int n) {
  int8_t lhs[CAPACITY];
  int8_t rhs[CAPACITY];
  int8_t true_values[CAPACITY];
  int8_t false_values[CAPACITY];
  int8_t out[CAPACITY];
  int8_t sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    lhs[i] = wrap_i8((i % 5) * 17 - 40);
    rhs[i] = wrap_i8((i % 7) * 11 - 35);
    true_values[i] = wrap_i8(90 - i * 3);
    false_values[i] = wrap_i8(-80 + i * 2);
    sentinel[i] = wrap_i8(33 + i);
    out[i] = sentinel[i];
  }

  if (select_gt_i8(lhs, rhs, true_values, false_values, out, n) != 0)
    return 1;
  for (int i = 0; i < n; ++i) {
    int8_t expected = lhs[i] > rhs[i] ? true_values[i] : false_values[i];
    if (out[i] != expected)
      return 10 + i;
  }
  for (int i = n; i < CAPACITY; ++i) {
    if (out[i] != sentinel[i])
      return 80 + i;
  }
  return 0;
}

static int run_i64_select_one(int n) {
  int64_t lhs[CAPACITY];
  int64_t rhs[CAPACITY];
  int64_t true_values[CAPACITY];
  int64_t false_values[CAPACITY];
  int64_t out[CAPACITY];
  int64_t sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    lhs[i] = (i % 5) * 1000000000LL - 2000000000LL + i;
    rhs[i] = (i % 7) * 700000000LL - 1500000000LL - i;
    true_values[i] = 80000000000LL + i * 17;
    false_values[i] = -81000000000LL - i * 19;
    sentinel[i] = 91000000000LL + i;
    out[i] = sentinel[i];
  }

  if (select_gt_i64(lhs, rhs, true_values, false_values, out, n) != 0)
    return 1;
  for (int i = 0; i < n; ++i) {
    int64_t expected = lhs[i] > rhs[i] ? true_values[i] : false_values[i];
    if (out[i] != expected)
      return 10 + i;
  }
  for (int i = n; i < CAPACITY; ++i) {
    if (out[i] != sentinel[i])
      return 80 + i;
  }
  return 0;
}

int main(void) {
  int lengths[] = {0, 1, 2, 3, 5, 8, 17, 31};
  int count = sizeof(lengths) / sizeof(lengths[0]);
  for (int i = 0; i < count; ++i) {
    int status = run_i16_add_one(0, lengths[i]);
    if (status != 0)
      return status;
    status = run_i16_add_one(2, lengths[i]);
    if (status != 0)
      return 100 + status;
    status = run_i16_add_one(4, lengths[i]);
    if (status != 0)
      return 200 + status;
    status = run_i8_add_one(lengths[i]);
    if (status != 0)
      return 300 + status;
    status = run_i64_add_one(lengths[i]);
    if (status != 0)
      return 400 + status;
    status = run_i8_copy_one(lengths[i]);
    if (status != 0)
      return 500 + status;
    status = run_i64_copy_one(lengths[i]);
    if (status != 0)
      return 600 + status;
    status = run_i8_select_one(lengths[i]);
    if (status != 0)
      return 700 + status;
    status = run_i64_select_one(lengths[i]);
    if (status != 0)
      return 800 + status;
  }
  printf("vector SEW demo n=31 i8/i16/i64 add/copy/select passed\n");
  return 0;
}
