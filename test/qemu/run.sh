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
extern int complex_vector_pipeline(int *a, int *b, int *tmp, int *out,
                                   int n, int factor);
extern int copy_then_sum(int *a, int *out, int n);
extern int vmul(int *a, int *b, int *c, int n);

static int run_case(int n, int factor) {
  int a[$rvv_capacity];
  int b[$rvv_capacity];
  int tmp[$rvv_capacity];
  int out[$rvv_capacity];
  int copied[$rvv_capacity];
  int multiplied[$rvv_capacity];

  for (int i = 0; i < $rvv_capacity; ++i) {
    a[i] = i + 1;
    b[i] = 10 + i * 2;
    tmp[i] = 0;
    out[i] = 0;
    copied[i] = 0;
    multiplied[i] = 0;
  }

  int sum = complex_vector_pipeline(a, b, tmp, out, n, factor);
  int expected = 0;
  for (int i = 0; i < n; ++i) {
    int tmp_expected = a[i] + b[i];
    int out_expected = tmp_expected * factor;
    if (tmp[i] != tmp_expected)
      return 10 + i;
    if (out[i] != out_expected)
      return 30 + i;
    expected += out_expected;
  }
  if (sum != expected)
    return 1;

  int copy_sum = copy_then_sum(a, copied, n);
  int expected_copy_sum = 0;
  for (int i = 0; i < n; ++i) {
    if (copied[i] != a[i])
      return 50 + i;
    expected_copy_sum += a[i];
  }
  if (copy_sum != expected_copy_sum)
    return 2;

  int mul_status = vmul(a, b, multiplied, n);
  if (mul_status != 0)
    return 3;
  for (int i = 0; i < n; ++i) {
    if (multiplied[i] != a[i] * b[i])
      return 70 + i;
  }

  return 0;
}

int main(void) {
  int lengths[] = {$rvv_lengths_c};
  int count = sizeof(lengths) / sizeof(lengths[0]);

  for (int i = 0; i < count; ++i) {
    int factor = 2 + (i % 3);
    int status = run_case(lengths[i], factor);
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
