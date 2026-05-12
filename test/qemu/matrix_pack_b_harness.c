#include <stdint.h>
#include <stdio.h>

extern int matmul_with_pack_i32(int *c, int *a, int *b, int *packed_b,
                                int rows, int cols, int inner);

#define CAPACITY 32

static uint32_t bits_i32(int value) { return (uint32_t)(int32_t)value; }

static int run_demo(void) {
  int a[6] = {1, 2, 3, 4, 5, 6};
  int b[6] = {7, 8, 9, 10, 11, 12};
  int packed_b[6] = {0, 0, 0, 0, 0, 0};
  int c[4] = {0, 0, 0, 0};

  int status = matmul_with_pack_i32(c, a, b, packed_b, 2, 2, 3);
  if (status != 0)
    return 1;

  if (packed_b[0] != 7 || packed_b[1] != 9 || packed_b[2] != 11 ||
      packed_b[3] != 8 || packed_b[4] != 10 || packed_b[5] != 12)
    return 2;

  if (c[0] != 58 || c[1] != 64 || c[2] != 139 || c[3] != 154)
    return 3;

  printf("matrix_pack_b demo packed=[%d %d %d; %d %d %d] result=[%d %d; %d %d]\n",
         packed_b[0], packed_b[1], packed_b[2], packed_b[3], packed_b[4],
         packed_b[5], c[0], c[1], c[2], c[3]);
  return 0;
}

static int run_case(int rows, int cols, int inner, int seed) {
  int a[CAPACITY];
  int b[CAPACITY];
  int packed_b[CAPACITY];
  int c[CAPACITY];
  int sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    a[i] = (i % 2 == 0) ? (seed + i + 3) : (-seed - i - 5);
    b[i] = (i % 3 == 0) ? (seed * 7 - i) : (-seed * 5 + i * 2);
    packed_b[i] = 900000 + i;
    sentinel[i] = -810000 - i;
    c[i] = sentinel[i];
  }

  int status = matmul_with_pack_i32(c, a, b, packed_b, rows, cols, inner);
  if (status != 0)
    return 10 + seed;

  for (int col = 0; col < cols; ++col) {
    for (int k = 0; k < inner; ++k) {
      int packed_index = col * inner + k;
      int original_index = k * cols + col;
      if (packed_b[packed_index] != b[original_index])
        return 50 + packed_index;
    }
  }

  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      uint32_t expected = 0;
      for (int k = 0; k < inner; ++k)
        expected += bits_i32(a[row * inner + k]) * bits_i32(b[k * cols + col]);
      int index = row * cols + col;
      if (bits_i32(c[index]) != expected)
        return 100 + index;
    }
  }

  for (int i = rows * cols; i < CAPACITY; ++i) {
    if (bits_i32(c[i]) != bits_i32(sentinel[i]))
      return 200 + i;
  }

  return 0;
}

int main(void) {
  int status = run_demo();
  if (status != 0)
    return status;

  status = run_case(2, 2, 3, 1);
  if (status != 0)
    return status;
  status = run_case(1, 4, 4, 2);
  if (status != 0)
    return status;
  status = run_case(3, 1, 2, 3);
  if (status != 0)
    return status;
  status = run_case(2, 3, 0, 4);
  if (status != 0)
    return status;

  return 0;
}
