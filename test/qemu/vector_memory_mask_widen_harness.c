#include <stdint.h>
#include <stdio.h>

extern int32_t vector_strided_load_demo(int32_t *out, int32_t *input,
                                        int32_t stride, int32_t n);
extern int32_t vector_indexed_load_demo(int32_t *out, int32_t *input,
                                        int32_t *indices, int32_t n);
extern int32_t vector_strided_store_demo(int32_t *base, int32_t *values,
                                         int32_t stride, int32_t n);
extern int32_t vector_strided_load_i8_demo(int8_t *out, int8_t *input,
                                           int32_t stride, int32_t n);
extern int32_t vector_strided_load_i16_demo(int16_t *out, int16_t *input,
                                            int32_t stride, int32_t n);
extern int32_t vector_strided_load_i64_demo(int64_t *out, int64_t *input,
                                            int32_t stride, int32_t n);
extern int32_t vector_strided_store_i8_demo(int8_t *base, int8_t *values,
                                            int32_t stride, int32_t n);
extern int32_t vector_strided_store_i16_demo(int16_t *base, int16_t *values,
                                             int32_t stride, int32_t n);
extern int32_t vector_strided_store_i64_demo(int64_t *base, int64_t *values,
                                             int32_t stride, int32_t n);
extern int32_t vector_indexed_store_demo(int32_t *base, int32_t *values,
                                         int32_t *indices, int32_t n);
extern int32_t vector_masked_strided_load_demo(int32_t *out, int32_t *input,
                                               int32_t *mask_lhs,
                                               int32_t *mask_rhs,
                                               int32_t *passthrough,
                                               int32_t stride, int32_t n);
extern int32_t vector_masked_indexed_load_demo(int32_t *out, int32_t *input,
                                               int32_t *indices,
                                               int32_t *mask_lhs,
                                               int32_t *mask_rhs,
                                               int32_t *passthrough, int32_t n);
extern int32_t vector_masked_strided_store_demo(int32_t *base, int32_t *values,
                                                int32_t *mask_lhs,
                                                int32_t *mask_rhs,
                                                int32_t stride, int32_t n);
extern int32_t vector_masked_indexed_store_demo(int32_t *base, int32_t *values,
                                                int32_t *indices,
                                                int32_t *mask_lhs,
                                                int32_t *mask_rhs, int32_t n);
extern int32_t vector_mask_logical_demo(int32_t *out, int32_t *lhs,
                                        int32_t *rhs, int32_t *x, int32_t *y,
                                        int32_t *passthrough, int32_t n);
extern int32_t vector_widen_add_i16_i32_demo(int32_t *out, int16_t *a,
                                             int16_t *b, int32_t n);

static int check_i32(const char *name, const int32_t *got,
                     const int32_t *expected, int n) {
  for (int i = 0; i < n; ++i) {
    if (got[i] != expected[i]) {
      fprintf(stderr, "%s mismatch at %d: got %d expected %d\n", name, i,
              got[i], expected[i]);
      return 1;
    }
  }
  return 0;
}

static int check_i8(const char *name, const int8_t *got, const int8_t *expected,
                    int n) {
  for (int i = 0; i < n; ++i) {
    if (got[i] != expected[i]) {
      fprintf(stderr, "%s mismatch at %d: got %d expected %d\n", name, i,
              (int)got[i], (int)expected[i]);
      return 1;
    }
  }
  return 0;
}

static int check_i16(const char *name, const int16_t *got,
                     const int16_t *expected, int n) {
  for (int i = 0; i < n; ++i) {
    if (got[i] != expected[i]) {
      fprintf(stderr, "%s mismatch at %d: got %d expected %d\n", name, i,
              (int)got[i], (int)expected[i]);
      return 1;
    }
  }
  return 0;
}

static int check_i64(const char *name, const int64_t *got,
                     const int64_t *expected, int n) {
  for (int i = 0; i < n; ++i) {
    if (got[i] != expected[i]) {
      fprintf(stderr, "%s mismatch at %d: got %lld expected %lld\n", name, i,
              (long long)got[i], (long long)expected[i]);
      return 1;
    }
  }
  return 0;
}

enum { STORE_N = 31, STORE_CAPACITY = 128 };

static int mask_true(int32_t lhs, int32_t rhs) { return lhs > rhs; }

static int run_strided_load_i8_case(int n) {
  int8_t input[STORE_CAPACITY];
  int8_t out[STORE_N];
  int8_t expected[STORE_N];
  int32_t stride = 3;
  for (int i = 0; i < STORE_CAPACITY; ++i)
    input[i] = (int8_t)(100 - i * 3);
  for (int i = 0; i < STORE_N; ++i) {
    out[i] = (int8_t)(-40 - i);
    expected[i] = out[i];
  }
  for (int i = 0; i < n; ++i)
    expected[i] = input[i * stride];

  if (vector_strided_load_i8_demo(out, input, stride, n) != 0)
    return 1;
  return check_i8("vector_strided_load_i8", out, expected, STORE_N);
}

static int run_strided_load_i16_case(int n) {
  int16_t input[STORE_CAPACITY];
  int16_t out[STORE_N];
  int16_t expected[STORE_N];
  int32_t stride = 3;
  for (int i = 0; i < STORE_CAPACITY; ++i)
    input[i] = (int16_t)(30000 - i * 17);
  for (int i = 0; i < STORE_N; ++i) {
    out[i] = (int16_t)(-12000 - i);
    expected[i] = out[i];
  }
  for (int i = 0; i < n; ++i)
    expected[i] = input[i * stride];

  if (vector_strided_load_i16_demo(out, input, stride, n) != 0)
    return 1;
  return check_i16("vector_strided_load_i16", out, expected, STORE_N);
}

static int run_strided_load_i64_case(int n) {
  int64_t input[STORE_CAPACITY];
  int64_t out[STORE_N];
  int64_t expected[STORE_N];
  int32_t stride = 3;
  for (int i = 0; i < STORE_CAPACITY; ++i)
    input[i] = 90000000000LL + i * 101;
  for (int i = 0; i < STORE_N; ++i) {
    out[i] = -91000000000LL - i;
    expected[i] = out[i];
  }
  for (int i = 0; i < n; ++i)
    expected[i] = input[i * stride];

  if (vector_strided_load_i64_demo(out, input, stride, n) != 0)
    return 1;
  return check_i64("vector_strided_load_i64", out, expected, STORE_N);
}

static int run_strided_store_i8_case(int n) {
  int8_t base[STORE_CAPACITY];
  int8_t expected[STORE_CAPACITY];
  int8_t values[STORE_N];
  int32_t stride = 3;
  for (int i = 0; i < STORE_CAPACITY; ++i) {
    base[i] = (int8_t)(-90 + i);
    expected[i] = base[i];
  }
  for (int i = 0; i < STORE_N; ++i)
    values[i] = (int8_t)(70 - i * 5);
  for (int i = 0; i < n; ++i)
    expected[i * stride] = values[i];

  if (vector_strided_store_i8_demo(base, values, stride, n) != 0)
    return 1;
  return check_i8("vector_strided_store_i8", base, expected, STORE_CAPACITY);
}

static int run_strided_store_i16_case(int n) {
  int16_t base[STORE_CAPACITY];
  int16_t expected[STORE_CAPACITY];
  int16_t values[STORE_N];
  int32_t stride = 3;
  for (int i = 0; i < STORE_CAPACITY; ++i) {
    base[i] = (int16_t)(-20000 + i * 3);
    expected[i] = base[i];
  }
  for (int i = 0; i < STORE_N; ++i)
    values[i] = (int16_t)(15000 - i * 29);
  for (int i = 0; i < n; ++i)
    expected[i * stride] = values[i];

  if (vector_strided_store_i16_demo(base, values, stride, n) != 0)
    return 1;
  return check_i16("vector_strided_store_i16", base, expected, STORE_CAPACITY);
}

static int run_strided_store_i64_case(int n) {
  int64_t base[STORE_CAPACITY];
  int64_t expected[STORE_CAPACITY];
  int64_t values[STORE_N];
  int32_t stride = 3;
  for (int i = 0; i < STORE_CAPACITY; ++i) {
    base[i] = -300000000000LL - i;
    expected[i] = base[i];
  }
  for (int i = 0; i < STORE_N; ++i)
    values[i] = 400000000000LL + i * 31;
  for (int i = 0; i < n; ++i)
    expected[i * stride] = values[i];

  if (vector_strided_store_i64_demo(base, values, stride, n) != 0)
    return 1;
  return check_i64("vector_strided_store_i64", base, expected, STORE_CAPACITY);
}

static int run_strided_store_case(int n) {
  int32_t base[STORE_CAPACITY];
  int32_t expected[STORE_CAPACITY];
  int32_t values[STORE_N];
  int32_t stride = 3;
  for (int i = 0; i < STORE_CAPACITY; ++i) {
    base[i] = -100000 - i;
    expected[i] = base[i];
  }
  for (int i = 0; i < STORE_N; ++i)
    values[i] = 5000 + i * 13;
  for (int i = 0; i < n; ++i)
    expected[i * stride] = values[i];

  if (vector_strided_store_demo(base, values, stride, n) != 0)
    return 1;
  return check_i32("vector_strided_store", base, expected, STORE_CAPACITY);
}

static int run_indexed_store_case(int n) {
  int32_t base[STORE_CAPACITY];
  int32_t expected[STORE_CAPACITY];
  int32_t values[STORE_N];
  int32_t indices[STORE_N];
  for (int i = 0; i < STORE_CAPACITY; ++i) {
    base[i] = -200000 - i;
    expected[i] = base[i];
  }
  for (int i = 0; i < STORE_N; ++i) {
    values[i] = -7000 + i * 17;
    indices[i] = (i * 7 + 3) % STORE_CAPACITY;
  }
  for (int i = 0; i < n; ++i)
    expected[indices[i]] = values[i];

  if (vector_indexed_store_demo(base, values, indices, n) != 0)
    return 1;
  return check_i32("vector_indexed_store", base, expected, STORE_CAPACITY);
}

static int run_masked_strided_load_case(int n) {
  int32_t input[STORE_CAPACITY];
  int32_t out[STORE_N];
  int32_t expected[STORE_N];
  int32_t mask_lhs[STORE_N];
  int32_t mask_rhs[STORE_N];
  int32_t passthrough[STORE_N];
  int32_t stride = 3;

  for (int i = 0; i < STORE_CAPACITY; ++i)
    input[i] = 3000 + i * 19;
  for (int i = 0; i < STORE_N; ++i) {
    out[i] = -300000 - i;
    expected[i] = out[i];
    mask_lhs[i] = (i % 5) + i;
    mask_rhs[i] = (i % 3) + 3;
    passthrough[i] = 90000 + i * 7;
  }
  for (int i = 0; i < n; ++i)
    expected[i] = mask_true(mask_lhs[i], mask_rhs[i]) ? input[i * stride]
                                                      : passthrough[i];

  if (vector_masked_strided_load_demo(out, input, mask_lhs, mask_rhs,
                                      passthrough, stride, n) != 0)
    return 1;
  return check_i32("vector_masked_strided_load", out, expected, STORE_N);
}

static int run_masked_indexed_load_case(int n) {
  int32_t input[STORE_CAPACITY];
  int32_t out[STORE_N];
  int32_t expected[STORE_N];
  int32_t indices[STORE_N];
  int32_t mask_lhs[STORE_N];
  int32_t mask_rhs[STORE_N];
  int32_t passthrough[STORE_N];

  for (int i = 0; i < STORE_CAPACITY; ++i)
    input[i] = -4000 + i * 23;
  for (int i = 0; i < STORE_N; ++i) {
    out[i] = -400000 - i;
    expected[i] = out[i];
    indices[i] = (i * 7 + 3) % STORE_CAPACITY;
    mask_lhs[i] = (i % 7) + 2;
    mask_rhs[i] = (i % 4) + 4;
    passthrough[i] = -91000 - i * 5;
  }
  for (int i = 0; i < n; ++i)
    expected[i] = mask_true(mask_lhs[i], mask_rhs[i]) ? input[indices[i]]
                                                      : passthrough[i];

  if (vector_masked_indexed_load_demo(out, input, indices, mask_lhs, mask_rhs,
                                      passthrough, n) != 0)
    return 1;
  return check_i32("vector_masked_indexed_load", out, expected, STORE_N);
}

static int run_masked_strided_store_case(int n) {
  int32_t base[STORE_CAPACITY];
  int32_t expected[STORE_CAPACITY];
  int32_t values[STORE_N];
  int32_t mask_lhs[STORE_N];
  int32_t mask_rhs[STORE_N];
  int32_t stride = 3;

  for (int i = 0; i < STORE_CAPACITY; ++i) {
    base[i] = -500000 - i;
    expected[i] = base[i];
  }
  for (int i = 0; i < STORE_N; ++i) {
    values[i] = 12000 + i * 31;
    mask_lhs[i] = (i % 6) + i;
    mask_rhs[i] = (i % 5) + 4;
  }
  for (int i = 0; i < n; ++i) {
    if (mask_true(mask_lhs[i], mask_rhs[i]))
      expected[i * stride] = values[i];
  }

  if (vector_masked_strided_store_demo(base, values, mask_lhs, mask_rhs, stride,
                                       n) != 0)
    return 1;
  return check_i32("vector_masked_strided_store", base, expected,
                   STORE_CAPACITY);
}

static int run_masked_indexed_store_case(int n) {
  int32_t base[STORE_CAPACITY];
  int32_t expected[STORE_CAPACITY];
  int32_t values[STORE_N];
  int32_t indices[STORE_N];
  int32_t mask_lhs[STORE_N];
  int32_t mask_rhs[STORE_N];

  for (int i = 0; i < STORE_CAPACITY; ++i) {
    base[i] = -600000 - i;
    expected[i] = base[i];
  }
  for (int i = 0; i < STORE_N; ++i) {
    values[i] = -13000 + i * 29;
    indices[i] = (i * 7 + 3) % STORE_CAPACITY;
    mask_lhs[i] = (i % 8) + 1;
    mask_rhs[i] = (i % 3) + 5;
  }
  for (int i = 0; i < n; ++i) {
    if (mask_true(mask_lhs[i], mask_rhs[i]))
      expected[indices[i]] = values[i];
  }

  if (vector_masked_indexed_store_demo(base, values, indices, mask_lhs,
                                       mask_rhs, n) != 0)
    return 1;
  return check_i32("vector_masked_indexed_store", base, expected,
                   STORE_CAPACITY);
}

int main(void) {
  enum { N = 17 };
  int store_lengths[] = {0, 1, 2, 3, 5, 8, 17, 31};
  int store_count = sizeof(store_lengths) / sizeof(store_lengths[0]);

  for (int i = 0; i < store_count; ++i) {
    if (run_strided_store_case(store_lengths[i]))
      return 10 + i;
    if (run_indexed_store_case(store_lengths[i]))
      return 20 + i;
    if (run_masked_strided_load_case(store_lengths[i]))
      return 30 + i;
    if (run_masked_indexed_load_case(store_lengths[i]))
      return 40 + i;
    if (run_masked_strided_store_case(store_lengths[i]))
      return 50 + i;
    if (run_masked_indexed_store_case(store_lengths[i]))
      return 60 + i;
    if (run_strided_load_i8_case(store_lengths[i]))
      return 70 + i;
    if (run_strided_load_i16_case(store_lengths[i]))
      return 80 + i;
    if (run_strided_load_i64_case(store_lengths[i]))
      return 90 + i;
    if (run_strided_store_i8_case(store_lengths[i]))
      return 100 + i;
    if (run_strided_store_i16_case(store_lengths[i]))
      return 110 + i;
    if (run_strided_store_i64_case(store_lengths[i]))
      return 120 + i;
  }

  int32_t strided_input[80];
  int32_t strided_out[N];
  int32_t strided_expected[N];
  for (int i = 0; i < 80; ++i)
    strided_input[i] = 1000 + i * 3;
  for (int i = 0; i < N; ++i) {
    strided_out[i] = -1;
    strided_expected[i] = strided_input[i * 3];
  }
  vector_strided_load_demo(strided_out, strided_input, 3, N);
  if (check_i32("vector_strided_load", strided_out, strided_expected, N))
    return 1;

  int32_t indexed_input[40];
  int32_t indices[N] = {5, 0, 3, 8, 13, 21, 1,  2, 34,
                        7, 6, 4, 9, 10, 11, 12, 14};
  int32_t indexed_out[N];
  int32_t indexed_expected[N];
  for (int i = 0; i < 40; ++i)
    indexed_input[i] = -200 + i * 11;
  for (int i = 0; i < N; ++i) {
    indexed_out[i] = -1;
    indexed_expected[i] = indexed_input[indices[i]];
  }
  vector_indexed_load_demo(indexed_out, indexed_input, indices, N);
  if (check_i32("vector_indexed_load", indexed_out, indexed_expected, N))
    return 2;

  int32_t lhs[N] = {5, 7, 7, 1, 9, 9, 2, 4, 12, 0, 3, 8, 8, 6, 10, 10, -1};
  int32_t rhs[N] = {3, 7, 8, 2, 1, 9, 5, 4, 11, 0, 4, 1, 8, 9, 7, 10, -2};
  int32_t x[N], y[N], passthrough[N], mask_out[N], mask_expected[N];
  for (int i = 0; i < N; ++i) {
    x[i] = 20 + i;
    y[i] = -3 * i;
    passthrough[i] = 700 + i;
    mask_out[i] = -1;
    mask_expected[i] = lhs[i] > rhs[i] ? x[i] + y[i] : passthrough[i];
  }
  vector_mask_logical_demo(mask_out, lhs, rhs, x, y, passthrough, N);
  if (check_i32("vector_mask_logical", mask_out, mask_expected, N))
    return 3;

  int16_t widen_a[N];
  int16_t widen_b[N];
  int32_t widen_out[N];
  int32_t widen_expected[N];
  for (int i = 0; i < N; ++i) {
    widen_a[i] = (int16_t)(i * 17 - 130);
    widen_b[i] = (int16_t)(200 - i * 9);
    widen_out[i] = -1;
    widen_expected[i] = (int32_t)widen_a[i] + (int32_t)widen_b[i];
  }
  vector_widen_add_i16_i32_demo(widen_out, widen_a, widen_b, N);
  if (check_i32("vector_widen_add_i16_i32", widen_out, widen_expected, N))
    return 4;

  printf("vector memory/mask/widen demo passed n=%d store_n=%d "
         "masked_nonunit_n=%d strided_sew_n=%d\n",
         N, STORE_N, STORE_N, STORE_N);
  return 0;
}
