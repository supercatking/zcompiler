#include <stddef.h>

void vector_add_i32_scalar(const int *a, const int *b, int *c, size_t n) {
  for (size_t i = 0; i < n; ++i)
    c[i] = a[i] + b[i];
}
