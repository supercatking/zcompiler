#include <stddef.h>
#include <riscv_vector.h>

void vector_add_i32(const int *a, const int *b, int *c, size_t n) {
  size_t i = 0;
  while (i < n) {
    size_t vl = __riscv_vsetvl_e32m1(n - i);
    vint32m1_t va = __riscv_vle32_v_i32m1(a + i, vl);
    vint32m1_t vb = __riscv_vle32_v_i32m1(b + i, vl);
    vint32m1_t vc = __riscv_vadd_vv_i32m1(va, vb, vl);
    __riscv_vse32_v_i32m1(c + i, vc, vl);
    i += vl;
  }
}
