#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"
qemu_bin="${ZCOMPILER_QEMU_RISCV64:-/home/qemu/qemu/build-riscv64-user/qemu-riscv64}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

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
print_output="$("$qemu_bin" -cpu max "$tmp_dir/print_i32")"
print_status="$?"
set -e

if [ "$print_output" != "220" ]; then
  echo "expected print_i32 stdout to be 220, got: $print_output" >&2
  exit 1
fi

if [ "$print_status" -ne 220 ]; then
  echo "expected print_i32 exit status 220, got: $print_status" >&2
  exit 1
fi

"$zc_bin" "$source_root/examples/complex_vector_pipeline.zc" \
  --emit-riscv-asm > "$tmp_dir/complex_vector_pipeline.s"
cat > "$tmp_dir/complex_vector_pipeline_harness.c" <<'EOF'
extern int complex_vector_pipeline(int *a, int *b, int *tmp, int *out,
                                   int n, int factor);
extern int copy_then_sum(int *a, int *out, int n);

static int run_case(int n, int factor) {
  int a[17];
  int b[17];
  int tmp[17] = {0};
  int out[17] = {0};
  int copied[17] = {0};

  for (int i = 0; i < 17; ++i) {
    a[i] = i + 1;
    b[i] = 10 + i * 2;
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

  return 0;
}

int main(void) {
  int lengths[] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 16, 17};
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
  "$tmp_dir/complex_vector_pipeline_harness.c" \
  -o "$tmp_dir/complex_vector_pipeline"

"$qemu_bin" -cpu max "$tmp_dir/complex_vector_pipeline"

echo "qemu-riscv64 validation passed"
