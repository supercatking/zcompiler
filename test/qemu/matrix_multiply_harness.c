#include <stdint.h>
#include <stdio.h>

extern int matmul_i32(int *c, int *a, int *b, int rows, int cols, int inner);

#define CAPACITY 32

static uint32_t bits_i32(int value) { return (uint32_t)(int32_t)value; }

static int seed_a(int index, int case_id) {
  switch ((index + case_id) % 6) {
  case 0:
    return 2147483600 - index;
  case 1:
    return -2147483600 + index;
  case 2:
    return index + 3;
  case 3:
    return -17 - index;
  case 4:
    return 12345 + case_id * 11 + index;
  default:
    return -9876 - case_id * 7 - index;
  }
}

static int seed_b(int index, int case_id) {
  switch ((index * 3 + case_id) % 6) {
  case 0:
    return -9 - index;
  case 1:
    return 31 + index;
  case 2:
    return -2147483000 + index;
  case 3:
    return 2147483000 - index;
  case 4:
    return 7 + case_id + index;
  default:
    return -5 - case_id - index;
  }
}

static int run_demo(void) {
  int a[6] = {1, 2, 3, 4, 5, 6};
  int b[6] = {7, 8, 9, 10, 11, 12};
  int c[4] = {0, 0, 0, 0};

  int status = matmul_i32(c, a, b, 2, 2, 3);
  if (status != 0)
    return 1;

  if (c[0] != 58 || c[1] != 64 || c[2] != 139 || c[3] != 154)
    return 2;

  printf("matrix_multiply demo 2x3 * 3x2 = [%d %d; %d %d]\n",
         c[0], c[1], c[2], c[3]);
  return 0;
}

static int run_case(int rows, int cols, int inner, int case_id) {
  int a[CAPACITY];
  int b[CAPACITY];
  int c[CAPACITY];
  int sentinel[CAPACITY];

  for (int i = 0; i < CAPACITY; ++i) {
    a[i] = seed_a(i, case_id);
    b[i] = seed_b(i, case_id);
    sentinel[i] = -700000 - case_id * 101 - i;
    c[i] = sentinel[i];
  }

  int status = matmul_i32(c, a, b, rows, cols, inner);
  if (status != 0)
    return 10 + case_id;

  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      uint32_t expected = 0;
      for (int k = 0; k < inner; ++k) {
        uint32_t lhs = bits_i32(a[row * inner + k]);
        uint32_t rhs = bits_i32(b[k * cols + col]);
        expected += lhs * rhs;
      }
      int index = row * cols + col;
      if (bits_i32(c[index]) != expected)
        return 100 + case_id * 20 + index;
    }
  }

  for (int i = rows * cols; i < CAPACITY; ++i) {
    if (bits_i32(c[i]) != bits_i32(sentinel[i]))
      return 200 + case_id;
  }

  return 0;
}

int main(void) {
  int status = run_demo();
  if (status != 0)
    return status;

  status = run_case(2, 2, 3, 0);
  if (status != 0)
    return status;

  status = run_case(1, 4, 4, 1);
  if (status != 0)
    return status;

  status = run_case(3, 1, 2, 2);
  if (status != 0)
    return status;

  status = run_case(2, 3, 0, 3);
  if (status != 0)
    return status;

  status = run_case(0, 3, 2, 4);
  if (status != 0)
    return status;

  return 0;
}
