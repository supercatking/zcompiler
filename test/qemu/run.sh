#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"
qemu_bin="${ZCOMPILER_QEMU_RISCV64:-/home/qemu/qemu/build-riscv64-user/qemu-riscv64}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT
manifest="$source_root/test/qemu/rvv_execution_manifest.json"
manifest_env="$tmp_dir/qemu_manifest.env"

python3 "$source_root/test/qemu/manifest.py" "$manifest" \
  --emit-shell-env "$manifest_env"
# shellcheck disable=SC1090
. "$manifest_env"

if [ ! -x "$qemu_bin" ]; then
  echo "qemu-riscv64 validation skipped: $qemu_bin is not executable"
  exit 0
fi

if ! command -v riscv64-linux-gnu-gcc >/dev/null; then
  echo "qemu-riscv64 validation skipped: riscv64-linux-gnu-gcc was not found"
  exit 0
fi

"$zc_bin" "$source_root/examples/print_i32.zc" --emit-riscv-asm \
  > "$tmp_dir/print_i32.s"
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/print_i32.s" -o "$tmp_dir/print_i32"

set +e
print_output="$("$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/print_i32")"
print_status="$?"
set -e

if [ "$print_output" != "$print_expected_stdout" ]; then
  echo "expected print_i32 stdout to be $print_expected_stdout, got: $print_output" >&2
  exit 1
fi

if [ "$print_status" -ne "$print_expected_status" ]; then
  echo "expected print_i32 exit status $print_expected_status, got: $print_status" >&2
  exit 1
fi

"$zc_bin" "$source_root/examples/complex_vector_pipeline.zc" \
  --emit-riscv-asm > "$tmp_dir/complex_vector_pipeline.s"
"$zc_bin" "$source_root/examples/vector_mul.zc" \
  --emit-riscv-asm > "$tmp_dir/vector_mul.s"
cat > "$tmp_dir/complex_vector_pipeline_harness.c" <<EOF
#include <stdint.h>

extern int complex_vector_pipeline(int *a, int *b, int *tmp, int *out,
                                   int n, int factor);
extern int copy_then_sum(int *a, int *out, int n);
extern int vmul(int *a, int *b, int *c, int n);

static uint32_t bits_i32(int value) { return (uint32_t)(int32_t)value; }

static uint32_t add_i32_bits(int lhs, int rhs) {
  return bits_i32(lhs) + bits_i32(rhs);
}

static uint32_t mul_i32_bits(uint32_t lhs, int rhs) {
  return lhs * bits_i32(rhs);
}

static int seed_a(int i) {
  switch (i % 6) {
  case 0:
    return 2147483600 - i;
  case 1:
    return -2147483600 + i;
  case 2:
    return -(i + 2);
  case 3:
    return i + 1;
  case 4:
    return 123456789 + i;
  default:
    return -76543210 - i;
  }
}

static int seed_b(int i) {
  switch (i % 6) {
  case 0:
    return 100 + i;
  case 1:
    return -200 - i;
  case 2:
    return 17 + i;
  case 3:
    return -19 - i;
  case 4:
    return 37 + i;
  default:
    return 29 + i;
  }
}

static int run_case(int n, int factor) {
  int a[$rvv_capacity];
  int b[$rvv_capacity];
  int tmp[$rvv_capacity];
  int out[$rvv_capacity];
  int copied[$rvv_capacity];
  int multiplied[$rvv_capacity];

  for (int i = 0; i < $rvv_capacity; ++i) {
    a[i] = seed_a(i);
    b[i] = seed_b(i);
    tmp[i] = 0;
    out[i] = 0;
    copied[i] = 0;
    multiplied[i] = 0;
  }

  int sum = complex_vector_pipeline(a, b, tmp, out, n, factor);
  uint32_t expected = 0;
  for (int i = 0; i < n; ++i) {
    uint32_t tmp_expected = add_i32_bits(a[i], b[i]);
    uint32_t out_expected = mul_i32_bits(tmp_expected, factor);
    if (bits_i32(tmp[i]) != tmp_expected)
      return 10 + i;
    if (bits_i32(out[i]) != out_expected)
      return 30 + i;
    expected += out_expected;
  }
  if (bits_i32(sum) != expected)
    return 1;

  int copy_sum = copy_then_sum(a, copied, n);
  uint32_t expected_copy_sum = 0;
  for (int i = 0; i < n; ++i) {
    if (bits_i32(copied[i]) != bits_i32(a[i]))
      return 50 + i;
    expected_copy_sum += bits_i32(a[i]);
  }
  if (bits_i32(copy_sum) != expected_copy_sum)
    return 2;

  int mul_status = vmul(a, b, multiplied, n);
  if (mul_status != 0)
    return 3;
  for (int i = 0; i < n; ++i) {
    uint32_t mul_expected = bits_i32(a[i]) * bits_i32(b[i]);
    if (bits_i32(multiplied[i]) != mul_expected)
      return 70 + i;
  }

  return 0;
}

static int factor_for_case(int i) {
  switch (i % 4) {
  case 0:
    return -3;
  case 1:
    return 5;
  case 2:
    return -7;
  default:
    return 9;
  }
}

int main(void) {
  int lengths[] = {$rvv_lengths_c};
  int count = sizeof(lengths) / sizeof(lengths[0]);

  for (int i = 0; i < count; ++i) {
    int status = run_case(lengths[i], factor_for_case(i));
    if (status != 0)
      return 100 + i;
  }

  return 0;
}

EOF

riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/complex_vector_pipeline.s" \
  "$tmp_dir/vector_mul.s" \
  "$tmp_dir/complex_vector_pipeline_harness.c" \
  -o "$tmp_dir/complex_vector_pipeline"

"$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/complex_vector_pipeline"

echo "qemu-riscv64 validation passed"
