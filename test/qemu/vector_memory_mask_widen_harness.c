#include <stdint.h>
#include <stdio.h>

extern int32_t vector_strided_load_demo(int32_t *out, int32_t *input,
                                        int32_t stride, int32_t n);
extern int32_t vector_indexed_load_demo(int32_t *out, int32_t *input,
                                        int32_t *indices, int32_t n);
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

int main(void) {
  enum { N = 17 };

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
  int32_t indices[N] = {5, 0, 3, 8, 13, 21, 1, 2, 34,
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

  printf("vector memory/mask/widen demo passed n=%d\n", N);
  return 0;
}
